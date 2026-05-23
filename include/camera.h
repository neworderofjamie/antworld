#pragma once

// BoB robotics includes
#include "renderer.h"

// SFML
#include <SFML/Graphics.hpp>

// Standard C++ includes
#include <memory>

namespace cv
{
class Size;
}

namespace AntWorld {
class Camera
{
    using degree_t = units::angle::degree_t;
    using meter_t = units::length::meter_t;

public:
    Camera(sf::Window &window,
           Renderer &renderer,
           const cv::Size &renderSize);

    void display();
    sf::Window &getWindow() const;
    bool isOpen() const;
    void setPosition(meter_t x, meter_t y, meter_t z);
    void setAttitude(degree_t yaw, degree_t pitch, degree_t roll);

    // Virtuals
    bool readFrame(cv::Mat &outFrame);

    static std::unique_ptr<sf::Window> initialiseWindow(const cv::Size &size);

private:
    meter_t m_PoseX;
    meter_t m_PoseY;
    meter_t m_PoseZ;
    degree_t m_PoseYaw;
    degree_t m_PosePitch;
    degree_t m_PoseRoll;
    
    sf::Window &m_Window;
    Renderer &m_Renderer;

    void update();
}; // Camera
} // AntWorld
