/// 
///  OpaqueVoxelRenderStage.h
///  Seed of Andromeda
///
///  Created by Benjamin Arnold on 1 Nov 2014
///  Copyright 2014 Regrowth Studios
///  All Rights Reserved
///  
///  This file implements the render stage for opaque voxels.
///  Opaque voxels have no transparency.
///

#pragma once

#ifndef OpaqueVoxelRenderStage_h__
#define OpaqueVoxelRenderStage_h__

#include "IRenderStage.h"

#include <Vorb/graphics/GLProgram.h>

class ChunkRenderer;
class GameRenderParams;
class MeshManager;

DECL_VG(template <class T>
        class Camera3D)

class OpaqueVoxelRenderStage : public IRenderStage
{
public:
    void hook(ChunkRenderer* renderer, const GameRenderParams* gameRenderParams);

    /// Draws the render stage
    virtual void render(const vg::Camera3D<f64>* camera) override;
private:
    ChunkRenderer* m_renderer;
    const GameRenderParams* m_gameRenderParams; ///< Handle to some shared parameters
};

#endif // OpaqueVoxelRenderStage_h__