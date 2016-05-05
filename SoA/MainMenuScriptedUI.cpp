#include "stdafx.h"
#include "MainMenuScriptedUI.h"
#include "SoaEngine.h"
#include "SoAState.h"
#include "MainMenuScreen.h"
#include "MainMenuSystemViewer.h"

#include <Vorb/ui/FormScriptEnvironment.h>
#include <Vorb/script/Environment.h>
#include <Vorb/Events.hpp>
#include <Vorb/ui/Form.h>
#include <Vorb/ui/KeyStrings.h>

#define ON_TARGET_CHANGE_NAME "onTargetChange"

MainMenuScriptedUI::MainMenuScriptedUI() {
    // Empty
}

MainMenuScriptedUI::~MainMenuScriptedUI() {
    // Empty
}

void MainMenuScriptedUI::init(const nString& startFormPath, vui::IGameScreen* ownerScreen, const vui::GameWindow* window, const f32v4& destRect, vg::SpriteFont* defaultFont /*= nullptr*/) {
    MainMenuScreen* mainMenuScreen = ((MainMenuScreen*)ownerScreen);
    m_inputMapper = mainMenuScreen->m_inputMapper;

    mainMenuScreen->m_mainMenuSystemViewer->TargetChange += makeDelegate(*this, &MainMenuScriptedUI::onTargetChange);

    ScriptedUI::init(startFormPath, ownerScreen, window, destRect, defaultFont); 
}

void MainMenuScriptedUI::registerScriptValues(vui::FormScriptEnvironment* newFormEnv) {
    vui::ScriptedUI::registerScriptValues(newFormEnv);
    vscript::Environment* env = newFormEnv->getEnv();

    SoaEngine::optionsController.registerScripting(env);

    // Controls menu stuff
    env->setNamespaces("Controls");
    env->addCRDelegate("size", makeRDelegate(*this, &MainMenuScriptedUI::getNumInputs));
    env->addCRDelegate("getInput", makeRDelegate(*this, &MainMenuScriptedUI::getInput));
    env->addCRDelegate("getKey", makeRDelegate(*this, &MainMenuScriptedUI::getKey));
    env->addCRDelegate("getDefaultKey", makeRDelegate(*this, &MainMenuScriptedUI::getDefaultKey));
    env->addCRDelegate("getKeyString", makeRDelegate(*this, &MainMenuScriptedUI::getKeyString));
    env->addCRDelegate("getDefaultKeyString", makeRDelegate(*this, &MainMenuScriptedUI::getDefaultKeyString));
    env->addCRDelegate("getName", makeRDelegate(*this, &MainMenuScriptedUI::getName));

    env->setNamespaces();
    env->addCDelegate("newGame", makeDelegate(*this, &MainMenuScriptedUI::newGame));

    env->setNamespaces("Game");
    env->addCDelegate("exit", makeDelegate(*this, &MainMenuScriptedUI::onExit));

    // TODO(Ben): Expose and use ECS instead???
    env->setNamespaces("Space");
    env->addCRDelegate("getTargetBody", makeRDelegate(*this, &MainMenuScriptedUI::getTargetBody));
    env->addCRDelegate("getBodyName", makeRDelegate(*this, &MainMenuScriptedUI::getBodyName));
    env->addCRDelegate("getBodyParentName", makeRDelegate(*this, &MainMenuScriptedUI::getBodyName));
    env->addCRDelegate("getBodyTypeName", makeRDelegate(*this, &MainMenuScriptedUI::getBodyTypeName));
    env->addCRDelegate("getBodyMass", makeRDelegate(*this, &MainMenuScriptedUI::getBodyMass));
    env->addCRDelegate("getBodyDiameter", makeRDelegate(*this, &MainMenuScriptedUI::getBodyDiameter));
    env->addCRDelegate("getBodyRotPeriod", makeRDelegate(*this, &MainMenuScriptedUI::getBodyRotPeriod));
    env->addCRDelegate("getBodyOrbPeriod", makeRDelegate(*this, &MainMenuScriptedUI::getBodyOrbPeriod));
    env->addCRDelegate("getBodyAxialTilt", makeRDelegate(*this, &MainMenuScriptedUI::getBodyAxialTilt));
    env->addCRDelegate("getBodyEccentricity", makeRDelegate(*this, &MainMenuScriptedUI::getBodyEccentricity));
    env->addCRDelegate("getBodyInclination", makeRDelegate(*this, &MainMenuScriptedUI::getBodyInclination));
    env->addCRDelegate("getBodySemiMajor", makeRDelegate(*this, &MainMenuScriptedUI::getBodySemiMajor));
    env->addCRDelegate("getGravityAccel", makeRDelegate(*this, &MainMenuScriptedUI::getGravityAccel));
    env->addCRDelegate("getVolume", makeRDelegate(*this, &MainMenuScriptedUI::getVolume));
    env->addCRDelegate("getAverageDensity", makeRDelegate(*this, &MainMenuScriptedUI::getAverageDensity));
    env->setNamespaces();
}

size_t MainMenuScriptedUI::getNumInputs() {
    return m_inputMapper->getInputLookup().size();
}

InputMapper::InputID MainMenuScriptedUI::getInput(int index) {
    // This is slow, but that is ok.
    auto& it = m_inputMapper->getInputLookup().begin();
    std::advance(it, index);
    return it->second;
}

VirtualKey MainMenuScriptedUI::getKey(InputMapper::InputID id) {
    return m_inputMapper->getKey(id);
}

VirtualKey MainMenuScriptedUI::getDefaultKey(InputMapper::InputID id) {
    return m_inputMapper->get(id).defaultKey;
}

nString MainMenuScriptedUI::getKeyString(InputMapper::InputID id) {
    return nString(VirtualKeyStrings[m_inputMapper->getKey(id)]);
}

nString MainMenuScriptedUI::getDefaultKeyString(InputMapper::InputID id) {
    return nString(VirtualKeyStrings[m_inputMapper->get(id).defaultKey]);
}

nString MainMenuScriptedUI::getName(InputMapper::InputID id) {
    return m_inputMapper->get(id).name;
}

void MainMenuScriptedUI::onExit(int code) {
    ((MainMenuScreen*)m_ownerScreen)->onQuit(this, code);
}

void MainMenuScriptedUI::onTargetChange(Sender s, vecs::EntityID id) {
    // TODO(Ben): Race condition???
    for (auto& it : m_forms) {
        if (it.first->isEnabled()) {
            vscript::Environment* env = it.second->getEnv();
            const vscript::Function& f = (*env)[ON_TARGET_CHANGE_NAME];
            if (!f.isNil()) f(id);
        }
    }
}

void MainMenuScriptedUI::newGame() {
    ((MainMenuScreen*)m_ownerScreen)->m_newGameClicked = true;
}

vecs::EntityID MainMenuScriptedUI::getTargetBody() {
    return ((MainMenuScreen*)m_ownerScreen)->m_mainMenuSystemViewer->getTargetBody();
}

nString MainMenuScriptedUI::getBodyName(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return state->spaceSystem->spaceBody.getFromEntity(entity).name;
}

nString MainMenuScriptedUI::getBodyParentName(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    auto parentID = state->spaceSystem->spaceBody.getFromEntity(entity).parentBodyComponent;
    if (parentID == 0) return "None";
    auto cmpID = state->spaceSystem->spaceBody.get(parentID).parentBodyComponent;
    return state->spaceSystem->spaceBody.get(cmpID).name;
}

nString MainMenuScriptedUI::getBodyTypeName(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    auto t = state->spaceSystem->spaceBody.getFromEntity(entity).type;
    nString n;
    switch (t) {
        case SpaceBodyType::BARYCENTER:
            n = "Barycenter"; break;
        case SpaceBodyType::STAR: // TODO(Ben): Spectral classes
            n = "Star"; break;
        case SpaceBodyType::PLANET:
            n = "Planet"; break;
        case SpaceBodyType::DWARF_PLANET:
            n = "Dwarf Planet"; break;
        case SpaceBodyType::MOON:
            n = "Moon"; break;
        case SpaceBodyType::DWARF_MOON:
            n = "Dwarf Moon"; break;
        case SpaceBodyType::ASTEROID:
            n = "Asteroid"; break;
        case SpaceBodyType::COMET:
            n = "Comet"; break;
        default:
            n = "UNKNOWN"; break;
    }
    return n;
}

f32 MainMenuScriptedUI::getBodyMass(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).mass;
}

f32 MainMenuScriptedUI::getBodyDiameter(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).diameter;
}

f32 MainMenuScriptedUI::getBodyRotPeriod(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).axisPeriod;
}

f32 MainMenuScriptedUI::getBodyOrbPeriod(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).t;
}

f32 MainMenuScriptedUI::getBodyEccentricity(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).e;
}

f32 MainMenuScriptedUI::getBodyInclination(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).i;
}

f32 MainMenuScriptedUI::getBodySemiMajor(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    return (f32)state->spaceSystem->spaceBody.getFromEntity(entity).major;
}

f32 MainMenuScriptedUI::getGravityAccel(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    auto& cmp = state->spaceSystem->spaceBody.getFromEntity(entity);
    f32 rad = (f32)(cmp.diameter * M_PER_KM / 2.0);
    return (f32)(M_G * cmp.mass / (rad * rad));
}

f32 MainMenuScriptedUI::getVolume(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    // TODO(Ben): Handle oblateness
    auto& cmp = state->spaceSystem->spaceBody.getFromEntity(entity);
    f32 rad = (f32)(cmp.diameter * M_PER_KM / 2.0);
    return (f32)(4.0 / 3.0 * M_PI * rad * rad * rad);
}

f32 MainMenuScriptedUI::getAverageDensity(vecs::EntityID entity) {
    SoaState* state = ((MainMenuScreen*)m_ownerScreen)->m_soaState;
    // TODO(Ben): This is a double lookup
    f32 volume = getVolume(entity);
    return (f32)(state->spaceSystem->spaceBody.getFromEntity(entity).mass / volume);
}