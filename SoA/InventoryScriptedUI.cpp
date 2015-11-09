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
    //env->setNamespaces("Inventory");
    //env->addCRDelegate("getItemList", makeRDelegate(*this, &InventoryScriptedUI::getItemList

    env->setNamespaces();
}

size_t InventoryScriptedUI::getNumInputs() {
    return m_inputMapper->getInputLookup().size();
}

InputMapper::InputID InventoryScriptedUI::getInput(int index) {
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
