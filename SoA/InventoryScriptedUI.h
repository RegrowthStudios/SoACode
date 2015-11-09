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

#include "Item.h"
#include "SoAState.h"

#include <Vorb/ui/ScriptedUI.h>

class InventoryScriptedUI : public vui::ScriptedUI {
public:
    InventoryScriptedUI();
    ~InventoryScriptedUI();

    void setSoaState(SoaState* soaState);
    void setInputMapper(InputMapper* inputMapper);
protected:
    virtual void registerScriptValues(vui::FormScriptEnvironment* newFormEnv) override;

    // LUA funcs for controls
    ui32 getNumInputs();
    InputMapper::InputID getInput(ui32 index);
    VirtualKey getKey(InputMapper::InputID id);
    VirtualKey getDefaultKey(InputMapper::InputID id);
    nString getKeyString(InputMapper::InputID id);
    nString getDefaultKeyString(InputMapper::InputID id);
    nString getName(InputMapper::InputID id);

    ui32 getInventorySize();
    ItemStack* getItem(ui32 index);

    nString getItemName(ItemStack* itemStack);
    ui32 getItemCount(ItemStack* itemStack);
    ui32 getItemMaxCount(ItemStack* itemStack);
    ui32 getItemWeight(ItemStack* itemStack);
    ui32 getItemValue(ItemStack* itemStack);
    ui32 getItemDurability(ItemStack* itemStack);
    ui32 getItemMaxDurability(ItemStack* itemStack);
    ItemPack* getItemPack(ItemStack* itemStack);

    nString getItemPackName(ItemPack* itemPack);

    InputMapper* m_inputMapper = nullptr;
    SoaState* m_soaState = nullptr;
};

#endif // InventoryScriptedUI_h__
