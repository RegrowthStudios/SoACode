///
/// CommonState.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 4 Jun 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Container for states and controllers passed between screens.
///

#pragma once

#ifndef CommonState_h__
#define CommonState_h__

#include "LoadContext.h"
#include <Vorb/VorbPreDecl.inl>
#include <Vorb/graphics/FullQuadVBO.h>

#include "BloomRenderStage.h"
#include "ColorFilterRenderStage.h"
#include "ExposureCalcRenderStage.h"
#include "HdrRenderStage.h"
#include "SkyboxRenderStage.h"
#include "SpaceSystemRenderStage.h"

struct SoaState;
DECL_VSOUND(class Engine)
DECL_VUI(class GameWindow)

/// Render stages that are used throughout the game.
struct CommonStateRenderStages{
    SkyboxRenderStage skybox;
    SpaceSystemRenderStage spaceSystem;
    HdrRenderStage hdr;
    ColorFilterRenderStage colorFilter;
    ExposureCalcRenderStage exposureCalc;
    BloomRenderStage bloom;
};

/// Client side state used throughout the game.
struct CommonState {
public:
    SoaState* state = nullptr;
    vsound::Engine* soundEngine = nullptr;
    vui::GameWindow* window = nullptr;
    StaticLoadContext loadContext;

    // TODO(Ben): Client side
    CommonStateRenderStages stages; // Shared render stages

    vg::FullQuadVBO quad; ///< Quad used for post-processing
};

#endif // CommonState_h__
