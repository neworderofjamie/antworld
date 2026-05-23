// Standard C++ includes
#include <filesystem>
#include <optional>
#include <sstream>

// PyBind11 includes
#include <pybind11/functional.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

// Plog
#include <plog/Log.h>
#include <plog/Severity.h>
#include <plog/Appenders/ConsoleAppender.h>
#include <plog/Formatters/TxtFormatter.h>

// Units
#include <units.h>

// Antworld includes
#include "camera.h"
#include "renderer.h"
#include "world.h"
w
using namespace pybind11::literals;


//----------------------------------------------------------------------------
// Anonymous namespace
//----------------------------------------------------------------------------
namespace
{
void initLogging(plog::Severity level)
{
    plog::init<plog::TxtFormatter>(level, plog::streamStdOut); 
}

void setFog(const std::string &mode, std::array<GLFloat, 3> colour, 
			GLfloat start, GLfloat end, GLfloat density)
{    
	// Set fog colour
	glFogfv (GL_FOG_COLOR, colour.data());
    
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

void setClearColour(GLfloat r, GLfloat g, GLfloat b) 
{
    glClearColor(r, g, b, a);
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
        m_Camera(*window, m_Renderer, renderSize)
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
            world.load(filepath,  { 0.0f, 1.0f, 0.0f },
                       { 0.898f, 0.718f, 0.353f }, clear);
		} 
		else if (ext == "obj") {
			world.loadObj(filepath, 1.0f, -1, GL_RGB, clear);
		} 
		else {
			throw std::runtime_error{ "Unknown file type" };
		}
		
		// Return world limits as a tuple of tuples: (xlim, ylim, zlim) = ...
		const auto &worldMin = world.getMaxBound();
		const auto &worldMax = world.getMinBound();
		return {
            {worldMin.x().value(), worldMax.x().value()},
            {worldMin.y().value(), worldMax.y().value()},
            {worldMin.z().value(), worldMax.z().value()}};
	}

	/*Agent_read_frame(AgentObject *self, PyObject *)
	{
		const auto size = self->members->agent.getOutputSize();

		// Allocate new numpy array
		npy_intp dims[3] = { size.height, size.width, 3 };
		auto array = PyArray_SimpleNew(3, dims, NPY_UINT8);
		if (!array)
			return nullptr;

		try {
			// A cv::Mat wrapper for the allocated data
			auto data = PyArray_DATA(reinterpret_cast<PyArrayObject *>(array));
			cv::Mat frame{ size.height, size.width, CV_8UC3, data };
			self->members->agent.readFrameSync(frame);
			BOB_ASSERT(frame.type() == CV_8UC3);
		} catch (std::exception &e) {
			Py_DECREF(array);
			PyErr_SetString(PyExc_RuntimeError, e.what());
			return nullptr;
		}

		// array is now populated with image data
		return array;
	}

	DLL_EXPORT PyObject *
	Agent_read_frame_greyscale(AgentObject *self, PyObject *)
	{
		const auto size = self->members->agent.getOutputSize();

		// Allocate new numpy array
		npy_intp dims[2] = { size.height, size.width };
		auto array = PyArray_SimpleNew(2, dims, NPY_UINT8);
		if (!array)
			return nullptr;

		try {
			// A cv::Mat wrapper for the allocated data
			auto data = PyArray_DATA(reinterpret_cast<PyArrayObject *>(array));
			cv::Mat frame{ size.height, size.width, CV_8UC1, data };
			self->members->agent.readGreyscaleFrameSync(frame);
			BOB_ASSERT(frame.type() == CV_8UC1);
		} catch (std::exception &e) {
			Py_DECREF(array);
			PyErr_SetString(PyExc_RuntimeError, e.what());
			return nullptr;
		}

		// array is now populated with image data
		return array;
	}*/

	void setPosition(const std::array<double, 3> &position)
	{
        using namespace units::distance;
        
		m_Camera.setPosition(meter_t{position[0]}, meter_t{position[1]}, meter_t{position[2]});
	}

	void setAttitude(const std::array<double, 3> &position)
	{
		using namespace units::angle;

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
	
	m.def("set_clear_colour", &setClearColour, pybind11::arg("r") = 0.75,
		  pybind11::arg("g") = 0.75, pybind11::arg("b") = 0.75);
	
	m.def("set_fog", &setFog, pybind11::arg("mode"), 
		  pybind11::arg("colour") = {0.75f, 0.75f, 0.75f, 1.0f},
		  pybind11::arg("start") = 0.0f, pybind11::arg("end") = 0.0f,
		  pybind11::arg("density") = 1.0f);
  
    //------------------------------------------------------------------------
    // fenn.Shape
    //------------------------------------------------------------------------
    pybind11::class_<AntAgent>(m, "AntAgent")
         .def(pybind11::init<const std::vector<size_t>&>())
         .def(pybind11::init<size_t>())

         .def("display", &AntAgent::display)
         .def("load_world", &AntAgent::loadWorld, pybind11::arg("filename"),
              pybind11::arg("clear") = true)
         .def("set_position", &AntAgent::setPosition)
         .def("set_attitude", &AntAgent::setAttitude);

}
