#pragma once

// OpenGL includes
#include <GL/glew.h>

//----------------------------------------------------------------------------
// AntWorld::OpenGLTypeTraits
//----------------------------------------------------------------------------
namespace AntWorld
{
template<typename T>
struct OpenGLTypeTraits
{
};

template<>
struct OpenGLTypeTraits<GLfloat>
{
    static constexpr GLenum type = GL_FLOAT;
};


template<>
struct OpenGLTypeTraits<GLuint>
{
    static constexpr GLenum type = GL_UNSIGNED_INT;
};

template<>
struct OpenGLTypeTraits<GLbyte>
{
    static constexpr GLenum type = GL_UNSIGNED_BYTE;
};

}   // namespace AntWorld
