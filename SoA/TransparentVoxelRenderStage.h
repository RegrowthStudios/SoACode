/// 
///  TransparentVoxelRenderStage.h
///  Seed of Andromeda
///
///  Created by Benjamin Arnold on 1 Nov 2014
///  Copyright 2014 Regrowth Studios
///  All Rights Reserved
///  
///  This file provides the implementation of the transparent voxel
///  render stage. Transparent voxels have partial transparency, and
///  will be sorted and blended.
///

#pragma once

#ifndef TransparentVoxelRenderStage_h__
#define TransparentVoxelRenderStage_h__

#include "IRenderStage.h"

#include <Vorb/graphics/GLProgram.h>

class ChunkRenderer;
class GameRenderParams;
class MeshManager;
DECL_VG(template <class T>
        class Camera3D)

class TransparentVoxelRenderStage : public IRenderStage {
public:
    void hook(ChunkRenderer* renderer, const GameRenderParams* gameRenderParams);

    /// Draws the render stage
    virtual void render(const vg::Camera3D<f64>* camera) override;
private:
    ChunkRenderer* m_renderer;
    const GameRenderParams* m_gameRenderParams = nullptr; ///< Handle to some shared parameters
};

#endif // TransparentVoxelRenderStage_h__

