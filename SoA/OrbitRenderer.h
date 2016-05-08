///
/// OrbitComponentRenderer.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 8 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Renders orbit components
///

#pragma once

#ifndef OrbitComponentRenderer_h__
#define OrbitComponentRenderer_h__

#include <Vorb/VorbPreDecl.inl>
#include <Vorb/graphics/gtypes.h>

class SpaceSystem;
struct SpaceBodyComponent;

DECL_VG(class GLProgram)

struct OrbitPathRenderData {
    VGBuffer vbo = 0; ///< vbo for the ellipse mesh
    VGBuffer vao = 0; ///< vao for the ellipse mesh
    ui32 numVerts = 0; ///< Number of vertices in the ellipse
    struct Vertex {
        f32v3 position;
        f32 angle;
    };
};

class OrbitRenderer {
public:
    /// Draws the ellipse
    /// Will lazily generate the mesh for renderData
    void drawPath(const SpaceBodyComponent& cmp, OrbitPathRenderData& renderData,
                  vg::GLProgram& colorProgram, const f32m4& WVP,
                  const f64v3& camPos,
                  const color4& color, const SpaceBodyComponent* parentCmp = nullptr);
private:
    void OrbitRenderer::generateOrbitEllipse(const SpaceBodyComponent& cmp, OrbitPathRenderData& renderData, vg::GLProgram& colorProgram);
};

#endif // OrbitComponentRenderer_h__