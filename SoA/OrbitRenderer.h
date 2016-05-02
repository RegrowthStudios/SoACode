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

class SpaceSystem;
struct SpaceBodyComponent;

DECL_VG(class GLProgram)

class OrbitRenderer {
public:
    /// Draws the ellipse
    void drawPath(SpaceBodyComponent& cmp, vg::GLProgram& colorProgram, const f32m4& WVP,
                  const f64v3& camPos, float blendFactor, SpaceBodyComponent* parentCmp = nullptr);
private:
    void OrbitRenderer::generateOrbitEllipse(SpaceBodyComponent& cmp, vg::GLProgram& colorProgram);
};

#endif // OrbitComponentRenderer_h__