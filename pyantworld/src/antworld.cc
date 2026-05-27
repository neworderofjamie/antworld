// Standard C++ includes
#include <array>
#include <filesystem>
#include <optional>
#include <sstream>
#include <tuple>

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
#include "render_mesh.h"
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

void setClearColour(const std::tuple<GLfloat, GLfloat, GLfloat, GLfloat> &colour) 
{
    glClearColor(std::get<0>(colour), std::get<1>(colour),
                 std::get<2>(colour), std::get<3>(colour));
}

class RenderMeshBuilder
{
public:
    virtual std::unique_ptr<AntWorld::RenderMesh> build() const = 0;
};

class RenderMeshCubeMapBuilder : public RenderMeshBuilder
{
public:
    RenderMeshCubeMapBuilder() = default;
    
    virtual std::unique_ptr<AntWorld::RenderMesh> build() const override final
    {
        return std::make_unique<AntWorld::RenderMeshCubeMap>();
    }
};

class RenderMeshSphericalBuilder : public RenderMeshBuilder
{
public:
    RenderMeshSphericalBuilder(double horizontalFOV, double verticalFOV, double startLongitude,
                               unsigned int numHorizontalSegments, unsigned int numVerticalSegments)
    :   m_HorizontalFOV(horizontalFOV), m_VerticalFOV(verticalFOV), m_StartLongitude(startLongitude),
        m_NumHorizontalSegments(numHorizontalSegments), m_NumVerticalSegments(numVerticalSegments)
    {}
    
    virtual std::unique_ptr<AntWorld::RenderMesh> build() const override final
    {
        return std::make_unique<AntWorld::RenderMeshSpherical>(m_HorizontalFOV, m_VerticalFOV, 
                                                               m_StartLongitude, m_NumHorizontalSegments,
                                                               m_NumVerticalSegments);
    }
    
private:
    degree_t m_HorizontalFOV;
    degree_t m_VerticalFOV;
    degree_t m_StartLongitude;
    unsigned int m_NumHorizontalSegments;
    unsigned int m_NumVerticalSegments;
};


class Agent
{
public:
    Agent(const std::tuple<int, int> &renderSize, const RenderMeshBuilder &renderMeshBuilder, 
          GLsizei cubemapSize, double nearClip, double farClip)
    :   m_Window{AntWorld::Camera::initialiseWindow(cv::Size(std::get<0>(renderSize), std::get<1>(renderSize)))},
        m_Renderer(std::move(renderMeshBuilder.build()), cubemapSize, nearClip, farClip),
        m_Camera(*m_Window, m_Renderer, cv::Size(std::get<0>(renderSize), std::get<1>(renderSize)))
    {}

    void display()
    {
        m_Camera.display();
    }

    std::array<std::tuple<double, double>, 3> loadWorld(const std::string &filename, bool clear)
    {
        auto &world = m_Renderer.getWorld();
        const auto ext = std::filesystem::path(filename).extension();

        if (ext == ".bin") {
            // Load with default world and ground colours
            world.load(filename, { 0.0f, 1.0f, 0.0f },
                       { 0.898f, 0.718f, 0.353f }, clear);
        } 
        else if (ext == ".obj") {
            world.loadObj(filename, 1.0f, -1, GL_RGB, clear);
        } 
        else {
            throw std::runtime_error{ "Unknown file type" };
        }
        
        // Return world limits as a tuple of tuples: (xlim, ylim, zlim) = ...
        const auto &worldMin = world.getMaxBound();
        const auto &worldMax = world.getMinBound();
        return {
            std::make_tuple(worldMin[0].value(), worldMax[0].value()),
            std::make_tuple(worldMin[1].value(), worldMax[1].value()),
            std::make_tuple(worldMin[2].value(), worldMax[2].value())};
    }
    
    pybind11::array_t<uint8_t> readFrame()
    {
        const auto &renderSize = m_Camera.getRenderSize();
        const size_t shape[3]{static_cast<size_t>(renderSize.height), static_cast<size_t>(renderSize.width), 3};
        const size_t strides[3]{static_cast<size_t>(renderSize.width) * 3, 3, 1};
        
        pybind11::array_t<uint8_t> array(shape, strides);
        
        cv::Mat frame{renderSize.height, renderSize.width, CV_8UC3, array.mutable_data()};
        m_Camera.readFrame(frame);
        assert(frame.type() == CV_8UC3);
        
        return array;
    }
   
    void setPosition(const std::tuple<double, double, double> &position)
    {
        m_Camera.setPosition(meter_t{std::get<0>(position)}, meter_t{std::get<1>(position)}, 
                             meter_t{std::get<2>(position)});
    }

    void setAttitude(const std::tuple<double, double, double> &attitude)
    {
        m_Camera.setAttitude(degree_t{std::get<0>(attitude)}, degree_t{std::get<1>(attitude)},
                             degree_t{std::get<2>(attitude)});
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
          pybind11::arg("colour") = std::make_tuple(0.75, 0.75, 0.75, 1.0));

    m.def("set_fog", &setFog, pybind11::arg("mode"), 
          pybind11::arg("colour") = std::make_tuple(0.75f, 0.75f, 0.75f, 1.0f),
          pybind11::arg("start") = 0.0f, pybind11::arg("end") = 0.0f,
          pybind11::arg("density") = 1.0f);
    
    //------------------------------------------------------------------------
    // RenderMeshBuilder
    //------------------------------------------------------------------------
    pybind11::class_<RenderMeshBuilder>(m, "RenderMeshBuilder");

    //------------------------------------------------------------------------
    // RenderMeshSphericalBuilderRenderMeshSphericalBuilder
    //------------------------------------------------------------------------
    pybind11::class_<RenderMeshSphericalBuilder, RenderMeshBuilder>(m, "RenderMeshSphericalBuilder")
        .def(pybind11::init<double, double, double, unsigned int, unsigned int>(), 
             pybind11::arg("horizontal_fov") = 360.0, pybind11::arg("vertical_fov") = 75.0, 
             pybind11::arg("start_longitude") = 15.0, pybind11::arg("num_horizontal_segments") = 40, 
             pybind11::arg("num_vertical_segments") = 10);
    
    //------------------------------------------------------------------------
    // RenderMeshCubeMapBuilder
    //------------------------------------------------------------------------
    pybind11::class_<RenderMeshCubeMapBuilder, RenderMeshBuilder>(m, "RenderMeshCubeMapBuilder")
        .def(pybind11::init<>());

    //------------------------------------------------------------------------
    // Agent
    //------------------------------------------------------------------------
    pybind11::class_<Agent>(m, "Agent")
        .def(pybind11::init<const std::tuple<int, int>&, const RenderMeshBuilder&, GLsizei, double, double>(),
             pybind11::arg("render_size"), pybind11::arg("render_mesh_builder"), pybind11::arg("cubemap_size") = 256,
             pybind11::arg("near_clip") = 0.001, pybind11::arg("far_clip") = 1000.0)

        .def("display", &Agent::display)
        .def("load_world", &Agent::loadWorld, pybind11::arg("filename"),
             pybind11::arg("clear") = true)
        .def("read_frame", &Agent::readFrame)
        .def("set_position", &Agent::setPosition)
        .def("set_attitude", &Agent::setAttitude);

}
