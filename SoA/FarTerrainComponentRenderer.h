///
/// FarTerrainComponentRenderer.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 22 Feb 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Renderer for far terrain patches
///

#pragma once

#ifndef FarTerrainComponentRenderer_h__
#define FarTerrainComponentRenderer_h__

#include <Vorb/VorbPreDecl.inl>
#include <Vorb/graphics/GLProgram.h>

DECL_VG(template <class T>
        class Camera3D)
struct AtmosphereComponent;
struct AxisRotationComponent;
struct FarTerrainComponent;
struct NamePositionComponent;
struct SpaceLightComponent;
struct SphericalTerrainComponent;

class FarTerrainComponentRenderer {
public:
    ~FarTerrainComponentRenderer();
    void initGL();
    void draw(const FarTerrainComponent& cmp,
              const vg::Camera3D<f64>* camera,
              const f64v3& lightDir,
              const f32 zCoef,
              const SpaceLightComponent* spComponent,
              const AxisRotationComponent* arComponent,
              const AtmosphereComponent* aComponent);
    void dispose();
private:
    void buildShaders();

    vg::GLProgram m_farTerrainProgram;
    vg::GLProgram m_farWaterProgram;
};

#endif // FarTerrainComponentRenderer_h__
