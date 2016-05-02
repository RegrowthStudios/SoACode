///
/// SphericalTerrainComponentRenderer.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 8 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Renders spherical terrain components
///

#pragma once

#ifndef SphericalTerrainComponentRenderer_h__
#define SphericalTerrainComponentRenderer_h__

#include <Vorb/VorbPreDecl.inl>
#include <Vorb/graphics/GLProgram.h>

class Camera;
struct AtmosphereComponent;
struct SpaceLightComponent;
struct SphericalTerrainComponent;
struct SpaceBodyComponent;

class SphericalTerrainComponentRenderer {
public:
    ~SphericalTerrainComponentRenderer();
    void initGL();
    void draw(SphericalTerrainComponent& cmp,
              const Camera* camera,
              const f32v3& lightDir,
              const f64v3& position,
              const f32 zCoef,
              const SpaceLightComponent* slComponent,
              const SpaceBodyComponent* bodyComponent,
              const AtmosphereComponent* aComponent);
    void dispose();
private:
    vg::GLProgram m_terrainProgram;
    vg::GLProgram m_waterProgram;
};

#endif // SphericalTerrainComponentRenderer_h__
