#pragma once

// OpenGL includes
#include <GL/glew.h>

// Antworld includes
#include "export.h"

// Forward declarations
namespace cv
{
    class Mat;
}

//------------------------------------------------------------------------
// AntWorld::Texture
//------------------------------------------------------------------------
namespace AntWorld
{
class ANTWORLD_EXPORT Texture
{
public:
    Texture();
    ~Texture();

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    void bind() const;
    void unbind() const;
    void upload(const cv::Mat &texture, GLint textureFormat);

private:
    //------------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    GLuint m_Texture;
};
}   // namespace AntWorld