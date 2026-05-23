#pragma once

// Units
#include <units.h>

// Antworld includes
#include "export.h"
#include "surface.h"

namespace AntWorld
{
class Render;
}

//----------------------------------------------------------------------------
// AntWorld::RenderMesh
//----------------------------------------------------------------------------
//! Class for generating geometry on which to render cubemap to screen
namespace AntWorld
{
class ANTWORLD_EXPORT RenderMesh
{
public:
    virtual ~RenderMesh()
    {
    }

    //------------------------------------------------------------------------
    // Public API
    //------------------------------------------------------------------------
    void render() const;

protected:
    RenderMesh()
    {
    }

    Surface &getSurface(){ return m_Surface; }

private:
    //-----------------------------------------------------------------------
    // Members
    //------------------------------------------------------------------------
    Surface m_Surface;
};

//----------------------------------------------------------------------------
// AntWorld::RenderMeshSpherical
//----------------------------------------------------------------------------
//! Class for sampling cubemap across a range of spherical coordiates defined by horizontal and vertical FOV,
//! converting these to cartesian coordinates and rendering them to grid with desired number of horizontal and vertical segments
ANTWORLD_EXPORT class RenderMeshSpherical : public RenderMesh
{
public:
    RenderMeshSpherical(units::angle::degree_t horizontalFOV, units::angle::degree_t verticalFOV, units::angle::degree_t startLongitude,
                        unsigned int numHorizontalSegments, unsigned int numVerticalSegments);
};

//----------------------------------------------------------------------------
// BoBRobotics::AntWorld::RenderMeshCubeMap
//----------------------------------------------------------------------------
//! Class for rendering cubemap straight back onto a unwrapped cube
ANTWORLD_EXPORT class RenderMeshCubeMap : public RenderMesh
{
public:
    RenderMeshCubeMap();
};
}   // namespace AntWorld
