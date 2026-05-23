#pragma once

// Standard C++ includes
#include <memory>

// OpenGL includes
#include <GL/glew.h>

// Units includes
#include <units.h>

// Antworld includes
#include "export.h"
#include "render_mesh.h"
#include "world.h"

namespace AntWorld
{
class RenderTarget;
}

//----------------------------------------------------------------------------
// AntWorld::Renderer
//----------------------------------------------------------------------------
//! Helper class which combines a world with a rendermesh to allow ant views of world to be rendered to screen
namespace AntWorld
{
class ANTWORLD_EXPORT Renderer
{
    using degree_t = units::angle::degree_t;
    using meter_t = units::length::meter_t;

public:
    static inline const struct SphericalRenderMesh{} sphericalRenderMesh;
    static inline const struct CubeMapRenderMesh{} cubeMapRenderMesh;

    // Legacy spherical cubemap constructor
    Renderer(GLsizei cubemapSize = 256, double nearClip = 0.001, double farClip = 1000.0,
             degree_t horizontalFOV = degree_t{360.0}, degree_t verticalFOV = degree_t{75.0});
        
    Renderer(SphericalRenderMesh, GLsizei cubemapSize = 256, double nearClip = 0.001, double farClip = 1000.0,
             degree_t horizontalFOV = degree_t{360.0}, degree_t verticalFOV = degree_t{75.0});
    Renderer(CubeMapRenderMesh, GLsizei cubemapSize = 256, double nearClip = 0.001, double farClip = 1000.0);

    virtual ~Renderer();


    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    void renderPanoramicView(meter_t x, meter_t y, meter_t z,
                             degree_t yaw, degree_t pitch, degree_t roll,
                             GLint viewportX, GLint viewportY, GLsizei viewportWidth, GLsizei viewportHeight,
                             GLuint drawFBO = 0);
    void renderPanoramicView(meter_t x, meter_t y, meter_t z,
                             degree_t yaw, degree_t pitch, degree_t roll,
                             RenderTarget &renderTarget, bool bind = true, bool clear = true);
    void renderFirstPersonView(meter_t x, meter_t y, meter_t z,
                               degree_t yaw, degree_t pitch, degree_t roll,
                               GLint viewportX, GLint viewportY, GLsizei viewportWidth, GLsizei viewportHeight);
    void renderFirstPersonView(meter_t x, meter_t y, meter_t z,
                               degree_t yaw, degree_t pitch, degree_t roll,
                               RenderTarget &renderTarget, bool bind = true, bool clear = true);
    void renderTopDownView(GLint viewportX, GLint viewportY, GLsizei viewportWidth, GLsizei viewportHeight);
    void renderTopDownView(RenderTarget &renderTarget, bool bind = true, bool clear = true);

    World &getWorld(){ return m_World; }
    const World &getWorld() const{ return m_World; }

protected:
    Renderer(std::unique_ptr<RenderMesh> renderMesh, GLsizei cubemapSize, 
             double nearClip, double farClip);
    
    //------------------------------------------------------------------------
    // Declared virtuals
    //------------------------------------------------------------------------
    virtual void renderPanoramicGeometry();
    virtual void renderFirstPersonGeometry();
    virtual void renderTopDownGeometry();

private:
    //------------------------------------------------------------------------
    // Private methods
    //------------------------------------------------------------------------
    void generateCubeFaceLookAtMatrices();
    void applyFrame(meter_t x, meter_t y, meter_t z,
                    degree_t yaw, degree_t pitch, degree_t roll);

    //------------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    World m_World;
    std::unique_ptr<RenderMesh> m_RenderMesh;

    GLuint m_CubemapTexture;
    GLuint m_FBO;
    GLuint m_DepthBuffer;
    GLfloat m_CubeFaceLookAtMatrices[6][16];

    const GLsizei m_CubemapSize;
    const double m_NearClip;
    const double m_FarClip;
};
}   // namespace AntWorld
