#include "stdafx.h"

#include "SpaceSystem.h"

#include <Vorb/TextureRecycler.hpp>
#include <Vorb/graphics/GLProgram.h>

SpaceSystem::SpaceSystem() : vecs::ECS() {

    addComponentTable(SPACE_SYSTEM_CT_SPACEBODY_NAME, &spaceBody);
    addComponentTable(SPACE_SYSTEM_CT_SPHERICALTERRAIN_NAME, &sphericalTerrain);
    addComponentTable(SPACE_SYSTEM_CT_GASGIANT_NAME, &gasGiant);
    addComponentTable(SPACE_SYSTEM_CT_STAR_NAME, &star);
    addComponentTable(SPACE_SYSTEM_CT_FARTERRAIN_NAME, &farTerrain);
    addComponentTable(SPACE_SYSTEM_CT_SPHERICALVOXEL_NAME, &sphericalVoxel);
    addComponentTable(SPACE_SYSTEM_CT_SPACELIGHT_NAME, &spaceLight);
    addComponentTable(SPACE_SYSTEM_CT_ATMOSPHERE_NAME, &atmosphere);
    addComponentTable(SPACE_SYSTEM_CT_PLANETRINGS_NAME, &planetRings);
    addComponentTable(SPACE_SYSTEM_CT_CLOUDS_NAME, &clouds);
}

SpaceSystem::~SpaceSystem() {

    // TODO: This sucks. Should be an ECS::dispose that does this automatically.
    for (auto& it : sphericalVoxel) {
        sphericalVoxel.disposeComponent(sphericalVoxel.getComponentID(it.first), it.first);
    }
    for (auto& it : spaceBody) {
        spaceBody.disposeComponent(spaceBody.getComponentID(it.first), it.first);
    }
    for (auto& it : sphericalVoxel) {
        sphericalVoxel.disposeComponent(sphericalVoxel.getComponentID(it.first), it.first);
    }
    for (auto& it : sphericalTerrain) {
        sphericalTerrain.disposeComponent(sphericalTerrain.getComponentID(it.first), it.first);
    }
}

vecs::ComponentID SpaceSystem::getComponent(const nString& name, vecs::EntityID eID) {
    auto& table = *getComponentTable(name);
    return table.getComponentID(eID);
}
