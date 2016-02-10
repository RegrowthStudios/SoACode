#include "stdafx.h"
#include "PauseMenuRenderStage.h"

#include "PauseMenu.h"

void PauseMenuRenderStage::hook(const PauseMenu* pauseMenu) {
    _pauseMenu = pauseMenu;
}

void PauseMenuRenderStage::render(const vg::Camera3D<f64>* camera /*= nullptr*/) {
    if (_pauseMenu->isOpen()) {
        _pauseMenu->draw();
    }
}