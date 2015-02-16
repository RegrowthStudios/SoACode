#include "stdafx.h"
#include "SoaController.h"

#include <Vorb/ecs/Entity.h>

#include "App.h"
#include "GameSystemAssemblages.h"
#include "GameSystemUpdater.h"
#include "SoaState.h"
#include "Options.h"
#include "SoaEngine.h"

SoaController::SoaController(const App* app) :
    m_app(app) {
    // Empty
}

SoaController::~SoaController() {
    // Empty
}

void SoaController::startGame(OUT SoaState* state) {
    // Load game ECS
    SoaEngine::GameSystemLoadData loadData;
    SoaEngine::loadGameSystem(state, loadData);

    GameSystem* gameSystem = state->gameSystem.get();
    SpaceSystem* spaceSystem = state->spaceSystem.get();

    if (state->isNewGame) {
        // Create the player entity
        state->playerEntity = GameSystemAssemblages::createPlayer(state->gameSystem.get(), state->startSpacePos,
                                          f64q(), 73.0f, f64v3(0.0), graphicsOptions.fov, m_app->getWindow().getAspectRatio());
        
        auto& svcmp = spaceSystem->m_sphericalVoxelCT.getFromEntity(state->startingPlanet);
        auto& arcmp = spaceSystem->m_axisRotationCT.getFromEntity(state->startingPlanet);
        auto& npcmp = spaceSystem->m_namePositionCT.getFromEntity(state->startingPlanet);

        auto& vpcmp = gameSystem->voxelPosition.getFromEntity(state->playerEntity);
        auto& spcmp = gameSystem->spacePosition.getFromEntity(state->playerEntity);

        f64v3 spacePos = state->startSpacePos;

        spcmp.position = arcmp.currentOrientation * spacePos + glm::normalize(arcmp.currentOrientation * spacePos) * 260.0 + npcmp.position;
        GameSystemUpdater::updateVoxelPlanetTransitions(gameSystem, spaceSystem, state);
    } else {
        // TODO(Ben): This
    }
}
