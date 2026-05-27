// BoB robotics includes
#include "camera.h"

// Standard C++ includes
#include <iostream>

// OpenCV includes
#include <opencv2/opencv.hpp>

// Plog
#include <plog/Log.h>

//----------------------------------------------------------------------------
// Anonymous namespace
//----------------------------------------------------------------------------
namespace
{
void handleGLError(GLenum, GLenum, GLuint, GLenum severity, GLsizei, const GLchar *message, const void *)
{
    if (severity == GL_DEBUG_SEVERITY_HIGH) {
        LOGE << message;
    }
    else if (severity == GL_DEBUG_SEVERITY_MEDIUM) {
        LOGW << message;
    }
    else if (severity == GL_DEBUG_SEVERITY_LOW) {
        LOGI << message;
    }
    else {
        LOGD << message;
    }
}
}

//----------------------------------------------------------------------------
// AntWorld::Camera
//----------------------------------------------------------------------------
namespace AntWorld 
{
Camera::Camera(sf::Window &window, Renderer &renderer, const cv::Size &renderSize)
 :  m_PoseX(0.0), m_PoseY(0.0), m_PoseZ(0.0), m_PoseYaw(0.0), m_PosePitch(0.0), m_PoseRoll(0.0), 
    m_RenderSize(renderSize), m_Window(window), m_Renderer(renderer)
{}
//----------------------------------------------------------------------------
sf::Window &Camera::getWindow() const
{
    return m_Window;
}
//----------------------------------------------------------------------------
void Camera::setPosition(meter_t x, meter_t y, meter_t z)
{
    m_PoseX = x;
    m_PoseY = y;
    m_PoseZ = z;
}
//----------------------------------------------------------------------------
void Camera::setAttitude(degree_t yaw, degree_t pitch, degree_t roll)
{
    m_PoseYaw = yaw;
    m_PosePitch = pitch;
    m_PoseRoll = roll;
}
//----------------------------------------------------------------------------
bool Camera::readFrame(cv::Mat &frame)
{
    // Render
    update();

    // Make sure frame is of right size and type
    frame.create(m_RenderSize, CV_8UC3);

    // Read pixels from framebuffer into outFrame
    // **TODO** it should be theoretically possible to go directly from frame buffer to GpuMat
    glReadPixels(0, 0, m_RenderSize.width, m_RenderSize.height,
                 GL_BGR, GL_UNSIGNED_BYTE, frame.data);

    // Flip image vertically
    cv::flip(frame, frame, 0);

    // Swap buffers
    m_Window.display();

    return true;
}
//----------------------------------------------------------------------------
void Camera::display()
{
    // Render
    update();

    // Swap buffers
    m_Window.display();
}
//----------------------------------------------------------------------------
void Camera::update()
{
    // Render to m_Window
    m_Window.setActive(true);

    // Clear colour and depth buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render first person
    m_Renderer.renderPanoramicView(m_PoseX, m_PoseY, m_PoseZ, 
                                   m_PoseYaw, m_PosePitch, 
                                   m_PoseRoll, 
                                   0, 0, m_RenderSize.width, m_RenderSize.height);
}
//----------------------------------------------------------------------------
bool Camera::isOpen() const
{
    return m_Window.isOpen();
}
//----------------------------------------------------------------------------
std::unique_ptr<sf::Window> Camera::initialiseWindow(const cv::Size &size)
{
    // Create SFML window
    // **YUCK**
    auto window = std::make_unique<sf::Window>(
        sf::VideoMode(
#if (SFML_VERSION_MAJOR == 3)
            sf::Vector2u(size.width, size.height)),
#elif (SFML_VERSION_MAJOR == 2)
            size.width, size.height),                                           
#else
    #error Unsupported SFML version
#endif
        "Ant world",
        sf::Style::Titlebar | sf::Style::Close);

    // Enable VSync
    window->setVerticalSyncEnabled(true);
    window->setActive(true);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        throw std::runtime_error("Failed to initialize GLEW");
    }

    glDebugMessageCallback(handleGLError, nullptr);

    // Set clear colour to match matlab and enable depth test
    glClearColor(0.75f, 0.75f, 0.75f, 1.0f);
    glEnable(GL_DEPTH_TEST);
    glLineWidth(4.0);
    glPointSize(4.0);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glEnable(GL_TEXTURE_2D);

    return window;
}

} // AntWorld
