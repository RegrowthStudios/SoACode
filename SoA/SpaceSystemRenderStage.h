///
/// SpaceSystemRenderStage.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 5 Dec 2014
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Render stage for drawing the space system
///

#pragma once

#ifndef SpaceSystemRenderStage_h__
#define SpaceSystemRenderStage_h__

#include "IRenderStage.h"
#include "GLProgram.h"

class SpaceSystem;

class SpaceSystemRenderStage : public vg::IRenderStage {
public:
    SpaceSystemRenderStage(const SpaceSystem* spaceSystem,
                           const Camera* camera,
                           vg::GLProgram* colorProgram);
    ~SpaceSystemRenderStage();

    /// Draws the render stage
    virtual void draw() override;
private:
    const SpaceSystem* m_spaceSystem;
    const Camera* m_camera;
    vg::GLProgram* m_colorProgram;
};

#endif // SpaceSystemRenderStage_h__