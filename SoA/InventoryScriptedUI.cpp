#include "stdafx.h"
#include "InventoryScriptedUI.h"

#include "SoaEngine.h"

#include <Vorb/ui/FormScriptEnvironment.h>
#include <Vorb/script/Environment.h>
#include <Vorb/Events.hpp>
#include <Vorb/ui/Form.h>
#include <Vorb/ui/KeyStrings.h>

InventoryScriptedUI::InventoryScriptedUI() {
    // Empty
}

InventoryScriptedUI::~InventoryScriptedUI() {
    // Empty
}

void InventoryScriptedUI::setSoaState(SoaState* soaState) {
    m_soaState = soaState;
}

void InventoryScriptedUI::setInputMapper(InputMapper* inputMapper) {
    m_inputMapper = inputMapper;
}

void InventoryScriptedUI::registerScriptValues(vui::FormScriptEnvironment* newFormEnv) {
    vui::ScriptedUI::registerScriptValues(newFormEnv);
    vscript::Environment* env = newFormEnv->getEnv();

    SoaEngine::optionsController.registerScripting(env);

    // Controls menu stuff
    env->setNamespaces("Controls");
    env->addCRDelegate("size", makeRDelegate(*this, &InventoryScriptedUI::getNumInputs));
    env->addCRDelegate("getInput", makeRDelegate(*this, &InventoryScriptedUI::getInput));
    env->addCRDelegate("getKey", makeRDelegate(*this, &InventoryScriptedUI::getKey));
    env->addCRDelegate("getDefaultKey", makeRDelegate(*this, &InventoryScriptedUI::getDefaultKey));
    env->addCRDelegate("getKeyString", makeRDelegate(*this, &InventoryScriptedUI::getKeyString));
    env->addCRDelegate("getDefaultKeyString", makeRDelegate(*this, &InventoryScriptedUI::getDefaultKeyString));
    env->addCRDelegate("getName", makeRDelegate(*this, &InventoryScriptedUI::getName));

    // TODO(Matthew): Extend scripting to get inventory information.
    env->setNamespaces("Inventory");
    env->addCRDelegate("getSize", makeRDelegate(*this, &InventoryScriptedUI::getInventorySize));
    env->addCRDelegate("getItem", makeRDelegate(*this, &InventoryScriptedUI::getItem));

    env->setNamespaces("Item");
    env->addCRDelegate("getName", makeRDelegate(*this, &InventoryScriptedUI::getItemName));
    env->addCRDelegate("getCount", makeRDelegate(*this, &InventoryScriptedUI::getItemCount));
    env->addCRDelegate("getMaxCount", makeRDelegate(*this, &InventoryScriptedUI::getItemMaxCount));
    env->addCRDelegate("getWeight", makeRDelegate(*this, &InventoryScriptedUI::getItemWeight));
    env->addCRDelegate("getValue", makeRDelegate(*this, &InventoryScriptedUI::getItemValue));
    env->addCRDelegate("getDurability", makeRDelegate(*this, &InventoryScriptedUI::getItemDurability));
    env->addCRDelegate("getMaxDurability", makeRDelegate(*this, &InventoryScriptedUI::getItemMaxDurability));
    env->addCRDelegate("getItemPack", makeRDelegate(*this, &InventoryScriptedUI::getItemPack));

    env->setNamespaces("ItemPack");
    env->addCRDelegate("getName", makeRDelegate(*this, &InventoryScriptedUI::getItemPackName));

    env->setNamespaces("Debug");
    env->addCDelegate("print", makeDelegate(*this, &InventoryScriptedUI::printMessage));

    env->setNamespaces();
}

ui32 InventoryScriptedUI::getNumInputs() {
    return m_inputMapper->getInputLookup().size();
}

InputMapper::InputID InventoryScriptedUI::getInput(ui32 index) {
    // This is slow, but that is ok.
    auto& it = m_inputMapper->getInputLookup().begin();
    std::advance(it, index);
    return it->second;
}

VirtualKey InventoryScriptedUI::getKey(InputMapper::InputID id) {
    return m_inputMapper->getKey(id);
}

VirtualKey InventoryScriptedUI::getDefaultKey(InputMapper::InputID id) {
    return m_inputMapper->get(id).defaultKey;
}

nString InventoryScriptedUI::getKeyString(InputMapper::InputID id) {
    return nString(VirtualKeyStrings[m_inputMapper->getKey(id)]);
}

nString InventoryScriptedUI::getDefaultKeyString(InputMapper::InputID id) {
    return nString(VirtualKeyStrings[m_inputMapper->get(id).defaultKey]);
}

nString InventoryScriptedUI::getName(InputMapper::InputID id) {
    return m_inputMapper->get(id).name;
}

ui32 InventoryScriptedUI::getInventorySize() {
    auto& invCmp = m_soaState->gameSystem->inventory.getFromEntity(m_soaState->clientState.playerEntity);
    return invCmp.items.size();
}

ItemStack* InventoryScriptedUI::getItem(ui32 index) {
    auto& invCmp = m_soaState->gameSystem->inventory.getFromEntity(m_soaState->clientState.playerEntity);
    return &invCmp.items[index];
}

// TODO(Matthew): Check if item exists else return 0.
nString InventoryScriptedUI::getItemName(ItemStack* itemStack) {
    return m_soaState->items[itemStack->id].name;
}

ui32 InventoryScriptedUI::getItemCount(ItemStack* itemStack) {
    return itemStack->count;
}

ui32 InventoryScriptedUI::getItemMaxCount(ItemStack* itemStack) {
    return m_soaState->items[itemStack->id].maxCount;
}

ui32 InventoryScriptedUI::getItemWeight(ItemStack* itemStack) {
    return m_soaState->items[itemStack->id].weight;
}

ui32 InventoryScriptedUI::getItemValue(ItemStack* itemStack) {
    return m_soaState->items[itemStack->id].value;
}

ui32 InventoryScriptedUI::getItemDurability(ItemStack* itemStack) {
    return itemStack->durability;
}

ui32 InventoryScriptedUI::getItemMaxDurability(ItemStack* itemStack) {
    return m_soaState->items[itemStack->id].maxDurability;
}

ItemPack* InventoryScriptedUI::getItemPack(ItemStack* itemStack) {
    return itemStack->pack;
}

nString InventoryScriptedUI::getItemPackName(ItemPack* itemPack) {
    return itemPack->name;
}

void InventoryScriptedUI::printMessage(nString message) {
    printf(message.c_str());
}