///
/// InventoryComponentRenderer.h
/// Seed of Andromeda
///
/// Created by Matthew Marshall on 08 November 2015
/// Copyright 2015 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Renderer for inventory.
///

#pragma once

#include "InventoryScriptedUI.h"

#ifndef InventoryComponentRenderer_h__
#define InventoryComponentRenderer_h__

class InventoryScriptedUI;

class InventoryComponentRenderer {
public:
    void init(InventoryScriptedUI* invUi);

    void render();
private:
    InventoryScriptedUI* m_ui;
};

#endif // InventoryComponentRenderer_h__
