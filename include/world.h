#pragma once

// Standard C++ includes
#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

// OpenGL includes
#include <GL/glew.h>

// Libantworld include
#include "export.h"
#include "surface.h"
#include "texture.h"

// Forward declarations
namespace cv
{
    class Mat;
}

namespace filesystem
{
    class path;
}

//----------------------------------------------------------------------------
// AntWorld::World
//----------------------------------------------------------------------------
//! Provides a means for loading a world stored on disk into OpenGL
namespace AntWorld
{
class ANTWORLD_EXPORT World
{
    using meter_t = units::length::meter_t;

public:
    World();
    World(const World&) = delete;
    World &operator=(const World &) = delete;

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    void render() const;
    void load(const std::filesystem::path &filename, const GLfloat (&worldColour)[3],
              const GLfloat (&groundColour)[3], bool clear = true);
    void loadObj(const std::filesystem::path &objFilename, float scale = 1.0f,
                 int maxTextureSize = -1, GLint textureFormat = GL_RGB, 
                 bool clear = true);

    const auto &getMinBound()
    {
        return m_MinBound;
    }

    const auto &getMaxBound()
    {
        return m_MaxBound;
    }

    void setMinBound(const std::array<meter_t, 3> &minBound){ m_MinBound = minBound; }
    void setMaxBound(const std::array<meter_t, 3> &maxBound){ m_MaxBound = maxBound; }

private:
    //------------------------------------------------------------------------
    // Private methods
    //------------------------------------------------------------------------
    void loadMaterials(const std::filesystem::path &basePath, const std::string &filename,
                       GLint textureFormat, int maxTextureSize,
                       std::map<std::string, std::tuple<Texture*, Surface::Colour>> &materialNames);

    //------------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    // Array of surfaces making up the model
    std::vector<Surface> m_Surfaces;

    /// Array of textures making up the model
    std::vector<std::unique_ptr<Texture>> m_Textures;

    // World bounds
    std::array<meter_t, 3> m_MinBound;
    std::array<meter_t, 3> m_MaxBound;
};
}   // namespace AntWorld
