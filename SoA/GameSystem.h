///
/// GameSystem.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 10 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Entity Component System for the main game entities.
///

#pragma once

#ifndef GameSystem_h__
#define GameSystem_h__

#include <Vorb/ComponentTable.hpp>
#include <Vorb/ECS.h>

#include "AabbCollidableComponent.h"
#include "MoveInputComponent.h"
#include "PhysicsComponent.h"
#include "SpacePositionComponent.h"
#include "VoxelPositionComponent.h"

class GameSystem : public vcore::ECS {
public:
    vcore::ComponentTable<AabbCollidableComponent> aabbCollidableCT;
    vcore::ComponentTable<MoveInputComponent> moveInputCT;
    vcore::ComponentTable<PhysicsComponent> physicsCT;
    vcore::ComponentTable<SpacePositionComponent> spacePositionCT;
    vcore::ComponentTable<VoxelPositionComponent> voxelPositionCT;
};

#endif // GameSystem_h__