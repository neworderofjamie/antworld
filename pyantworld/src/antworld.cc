// Standard C++ includes
#include <array>
#include <filesystem>
#include <optional>
#include <sstream>

// PyBind11 includes
#include <pybind11/functional.h>
#include <pybind11/numpy.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Plog
#include <plog/Log.h>
#include <plog/Formatters/TxtFormatter.h>
#include <plog/Initializers/ConsoleInitializer.h>

// OpenGL
#include <GL/glew.h>

// Units
#include <units.h>

// Antworld includes
#include "camera.h"
#include "renderer.h"
#include "world.h"

using namespace pybind11::literals;
using degree_t = units::angle::degree_t;
using meter_t = units::length::meter_t;

//----------------------------------------------------------------------------
// Anonymous namespace
//----------------------------------------------------------------------------
namespace
{
void initLogging(plog::Severity level)
{
    plog::init<plog::TxtFormatter>(level, plog::streamStdOut); 
}

void setFog(const std::string &mode, const std::array<GLfloat, 4> &colour, 
			GLfloat start, GLfloat end, GLfloat density)
{    
	// Set fog colour
	glFogfv(GL_FOG_COLOR, colour.data());
    
    // Set fog start, end and density
    glFogf(GL_FOG_START, start);
    glFogf(GL_FOG_END, end);
    glFogf(GL_FOG_DENSITY, density);
    
    // If fog is disabled, turn it off
    if(mode == "disabled") {
        glDisable(GL_FOG);
    }
    // Otherwise
    else {
        // Enable
        glEnable(GL_FOG);

        // Convert mode string to GL enumeration
        if(mode == "linear") {
            glFogi(GL_FOG_MODE, GL_LINEAR);
        }
        else if(mode == "exp") {
            glFogi(GL_FOG_MODE, GL_EXP);
        }
        else if(mode == "exp2") {
            glFogi(GL_FOG_MODE, GL_EXP2);
        }
        else {
            throw std::runtime_error("Unknown fog mode '" + mode + "'");
        }
    }
}

void setClearColour(const std::array<GLfloat, 4> &colour) 
{
    glClearColor(colour[0], colour[1], colour[2], colour[3]);
}


class AntAgent
{
public:
	AntAgent(const cv::Size &renderSize, GLsizei cubemapSize, 
             double nearClip, double farClip,
             double horizontalFOV, double verticalFOV)
	:	m_Window{AntWorld::Camera::initialiseWindow(renderSize)},
        m_Renderer(AntWorld::Renderer::sphericalRenderMesh, cubemapSize, nearClip, farClip, 
                   degree_t{horizontalFOV}, degree_t{verticalFOV}),
        m_Camera(*m_Window, m_Renderer, renderSize)
    {}

    AntAgent(const cv::Size &renderSize, GLsizei cubemapSize, 
             double nearClip, double farClip)
    :	m_Window{AntWorld::Camera::initialiseWindow(renderSize)},
		m_Renderer(AntWorld::Renderer::cubeMapRenderMesh, cubemapSize, nearClip, farClip),
        m_Camera(*m_Window, m_Renderer, renderSize)
    {}
	
	void display()
	{
		m_Camera.display();
	}
	
	std::array<std::array<double, 2>, 3> loadWorld(const std::string &filename, bool clear)
	{
		auto &world = m_Renderer.getWorld();
		const auto ext = std::filesystem::path(filename).extension();
		if (ext == "bin") {
            // Load with default world and ground colours
            world.load(filename, { 0.0f, 1.0f, 0.0f },
                       { 0.898f, 0.718f, 0.353f }, clear);
		} 
		else if (ext == "obj") {
			world.loadObj(filename, 1.0f, -1, GL_RGB, clear);
		} 
		else {
			throw std::runtime_error{ "Unknown file type" };
		}
		
		// Return world limits as a tuple of tuples: (xlim, ylim, zlim) = ...
		const auto &worldMin = world.getMaxBound();
		const auto &worldMax = world.getMinBound();
		return {
            std::array<double, 2>{worldMin[0].value(), worldMax[0].value()},
            std::array<double, 2>{worldMin[1].value(), worldMax[1].value()},
            std::array<double, 2>{worldMin[2].value(), worldMax[2].value()}};
	}
    
    pybind11::array_t<uint8_t> readFrame()
    {
        const auto &renderSize = m_Camera.getRenderSize();
        const size_t shape[3]{renderSize.height, renderSize.width, 3};
        const size_t strides[3]{renderSize.width * 3, 3, 1};
        
        pybind11::array_t<uint8_t> array(shape, strides);
        
        cv::Mat frame{renderSize.height, renderSize.width, CV_8UC3, array.mutable_data()};
        m_Camera.readFrame(frame);
        assert(frame.type() == CV_8UC3);
        
        return array;
    }
   
	void setPosition(const std::array<double, 3> &position)
	{
		m_Camera.setPosition(meter_t{position[0]}, meter_t{position[1]}, meter_t{position[2]});
	}

	void setAttitude(const std::array<double, 3> &position)
	{
		m_Camera.setAttitude(degree_t{position[0]}, degree_t{position[1]}, degree_t{position[2]});
	}
private:
    std::unique_ptr<sf::Window> m_Window;
    AntWorld::Renderer m_Renderer;
    AntWorld::Camera m_Camera;
};
}

//----------------------------------------------------------------------------
// _antworld
//----------------------------------------------------------------------------
PYBIND11_MODULE(_antworld, m) 
{
    //------------------------------------------------------------------------
    // Enumerations
    //------------------------------------------------------------------------
    pybind11::enum_<plog::Severity>(m, "PlogSeverity")
        .value("NONE", plog::Severity::none)
        .value("FATAL", plog::Severity::fatal)
        .value("ERROR", plog::Severity::error)
        .value("WARNING", plog::Severity::warning)
        .value("INFO", plog::Severity::info)
        .value("DEBUG", plog::Severity::debug)
        .value("VERBOSE", plog::Severity::verbose);
    
    
    //------------------------------------------------------------------------
    // Free functions
    //------------------------------------------------------------------------
    m.def("init_logging", &initLogging, pybind11::arg("level") = plog::Severity::info);
	
	m.def("set_clear_colour", &setClearColour, 
          pybind11::arg("colour") = std::array<GLfloat, 4>{0.75, 0.75, 0.75, 1.0});
	
	m.def("set_fog", &setFog, pybind11::arg("mode"), 
		  pybind11::arg("colour") = std::array<GLfloat, 4>{0.75f, 0.75f, 0.75f, 1.0f},
		  pybind11::arg("start") = 0.0f, pybind11::arg("end") = 0.0f,
		  pybind11::arg("density") = 1.0f);
  
    //------------------------------------------------------------------------
    // fenn.Shape
    //------------------------------------------------------------------------
    pybind11::class_<AntAgent>(m, "AntAgent")
         //.def(pybind11::init<const std::vector<size_t>&>())
         //.def(pybind11::init<size_t>())

         .def("display", &AntAgent::display)
         .def("load_world", &AntAgent::loadWorld, pybind11::arg("filename"),
              pybind11::arg("clear") = true)
         .def("set_position", &AntAgent::setPosition)
         .def("set_attitude", &AntAgent::setAttitude);

}
