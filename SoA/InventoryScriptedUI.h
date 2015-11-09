///
/// InventoryScriptedUI.h
/// Seed of Andromeda
///
/// Created by Matthew Marshall on 08 Nov 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Custom inventory UI
///

#pragma once

#ifndef InventoryScriptedUI_h__
#define InventoryScriptedUI_h__

#include "InputMapper.h"

#include <Vorb/ui/ScriptedUI.h>

class InventoryScriptedUI : public vui::ScriptedUI {
public:
    InventoryScriptedUI();
    ~InventoryScriptedUI();

    void setInputMapper(InputMapper* inputMapper);
protected:
    virtual void registerScriptValues(vui::FormScriptEnvironment* newFormEnv) override;

    // LUA funcs for controls
    size_t getNumInputs();
    InputMapper::InputID getInput(int index);
    VirtualKey getKey(InputMapper::InputID id);
    VirtualKey getDefaultKey(InputMapper::InputID id);
    nString getKeyString(InputMapper::InputID id);
    nString getDefaultKeyString(InputMapper::InputID id);
    nString getName(InputMapper::InputID id);

    InputMapper* m_inputMapper = nullptr;
};

#endif // InventoryScriptedUI_h__
