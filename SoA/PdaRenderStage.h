/// 
///  PdaRenderStage.h
///  Seed of Andromeda
///
///  Created by Benjamin Arnold on 2 Nov 2014
///  Copyright 2014 Regrowth Studios
///  All Rights Reserved
///  
///  This file implements the render stage for PDA rendering
///

#pragma once

#ifndef PdaRenderStage_h__
#define PdaRenderStage_h__

#include "IRenderStage.h"

class PDA;

class PdaRenderStage : public IRenderStage {
public:
    PdaRenderStage();

    void hook(const PDA* pda);

    /// Draws the render stage
    virtual void render(const vg::Camera3D<f64>* camera) override;
private:
    const PDA* _pda = nullptr; ///< Handle to the PDA
};

#endif // PdaRenderStage_h__