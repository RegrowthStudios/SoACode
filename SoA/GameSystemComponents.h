///
/// GameSystemComponents.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 11 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Components for game system
///

#pragma once

#ifndef GameSystemComponents_h__
#define GameSystemComponents_h__

#include "VoxelPlanetMapper.h"
#include <Vorb/Entity.h>

struct AabbCollidableComponent {
    f32v3 box = f32v3(0.0f); ///< x, y, z widths in blocks
    f32v3 offset = f32v3(0.0f); ///< x, y, z offsets in blocks
};

struct ParkourInputComponent {
    // Bitfield inputs
    union {
        struct {
            bool tryMoveForward : 1; ///< True when moving forward
            bool tryMoveBackward : 1; ///< True when moving backward
            bool tryMoveLeft : 1; ///< True when moving left
            bool tryMoveRight : 1; ///< True when moving right
            bool tryJump : 1; ///< True when attempting to jump
            bool tryCrouch : 1; ///< True when attempting to crouch
            bool tryParkour : 1; ///< True when parkouring
            bool trySprint : 1; ///< True when sprinting
        };
        ui8 moveFlags = 0;
    };
    vcore::ComponentID physicsComponent;
    float acceleration;
    float maxSpeed;
};

struct FreeMoveInputComponent {
    // Bitfield inputs
    union {
        struct {
            bool tryMoveForward : 1; ///< True when moving forward
            bool tryMoveBackward : 1; ///< True when moving backward
            bool tryMoveLeft : 1; ///< True when moving left
            bool tryMoveRight : 1; ///< True when moving right
            bool tryUp : 1; ///< True when attempting to go up
            bool tryDown : 1; ///< True when attempting to go down
        };
        ui8 moveFlags = 0;
    };
    vcore::ComponentID physicsComponent;
};

struct SpacePositionComponent {
    f64v3 position = f64v3(0.0);
    f64q orientation;
    vcore::ComponentID voxelPositionComponent = 0;
};

typedef f64v3 VoxelPosition;

struct VoxelPositionComponent {
    VoxelPosition position = VoxelPosition(0.0);
    f64q orientation;
    vvox::VoxelPlanetMapData mapData;
};

struct PhysicsComponent {
    vcore::ComponentID spacePositionComponent = 0;
    vcore::ComponentID voxelPositionComponent = 0;
    f64v3 velocity = f64v3(0.0);
    f32 mass;
};

#endif // GameSystemComponents_h__