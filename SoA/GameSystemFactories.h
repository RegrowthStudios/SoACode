///
/// GameSystemFactories.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 11 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Factory methods for GameSystem entities
///

#pragma once

#ifndef GameSystemFactories_h__
#define GameSystemFactories_h__

#include <Vorb/ecs/Entity.h>

class GameSystem;

#include "GameSystemComponents.h"

namespace GameSystemFactories {
    /************************************************************************/
    /* Entity Factories                                                     */
    /************************************************************************/
    /// Player entity
    extern vcore::EntityID createPlayer(OUT GameSystem* gameSystem, const f64v3& spacePosition,
                                      const f64q& orientation, float massKg, const f64v3& initialVel);
    extern void destroyPlayer(OUT GameSystem* gameSystem, vcore::EntityID playerEntity);

    /************************************************************************/
    /* Component Factories                                                  */
    /************************************************************************/
    /// Free movement component
    extern vcore::ComponentID addFreeMoveInput(OUT GameSystem* gameSystem, vcore::EntityID entity,
                                               vcore::ComponentID physicsComponent);
    extern void removeFreeMoveInput(OUT GameSystem* gameSystem, vcore::EntityID entity);
    /// Physics component
    extern vcore::ComponentID addPhysics(OUT GameSystem* gameSystem, vcore::EntityID entity,
                                         float massKg, const f64v3& initialVel,
                                         vcore::ComponentID spacePositionComponent,
                                         OPT vcore::ComponentID voxelPositionComponent = 0);
    extern void removePhysics(OUT GameSystem* gameSystem, vcore::EntityID entity);
    /// Space position component
    extern vcore::ComponentID addSpacePosition(OUT GameSystem* gameSystem, vcore::EntityID entity,
                                               const f64v3& position, const f64q& orientation);
    extern void removeSpacePosition(OUT GameSystem* gameSystem, vcore::EntityID entity);
    /// AABB Collision component
    extern vcore::ComponentID addAabbCollidable(OUT GameSystem* gameSystem, vcore::EntityID entity,
                                                const f32v3& box, const f32v3& offset);
    extern void removeAabbCollidable(OUT GameSystem* gameSystem, vcore::EntityID entity);
    /// Voxel Position Component
    extern vcore::ComponentID addVoxelPosition(OUT GameSystem* gameSystem, vcore::EntityID entity,
                                               vcore::ComponentID parentVoxelComponent,
                                               const VoxelPosition& position,
                                               const f64q& orientation,
                                               vvox::VoxelPlanetMapData mapData);
    extern void removeVoxelPosition(OUT GameSystem* gameSystem, vcore::EntityID entity);
}

#endif // GameSystemFactories_h__