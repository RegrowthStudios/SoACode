#include "stdafx.h"
#include "InventoryComponentRenderer.h"

void InventoryComponentRenderer::init(InventoryScriptedUI* invUi) {
    m_ui = invUi;
}

void InventoryComponentRenderer::render() {
    m_ui->draw();
}
