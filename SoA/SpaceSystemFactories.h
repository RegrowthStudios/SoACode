///
/// SpaceSystemFactories.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 13 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Component and entity factories for SpaceSystem
///

#pragma once

#ifndef SpaceSystemFactories_h__
#define SpaceSystemFactories_h__

class SpaceSystem;

#include <Vorb/Entity.h>
#include <Vorb/VorbPreDecl.inl>

class SoaState;
DECL_VVOX(class VoxelMapData);

namespace SpaceSystemFactories {
    /************************************************************************/
    /* Entity Factories                                                     */
    /************************************************************************/
   
    // TODO(Ben): Move SpaceSystem stuff here

    /************************************************************************/
    /* Component Factories                                                  */
    /************************************************************************/
    /// Spherical voxel component
    extern vcore::ComponentID addSphericalVoxelComponent(OUT SpaceSystem* spaceSystem, vcore::EntityID entity,
                                                         vcore::ComponentID sphericalTerrainComponent,
                                                         const vvox::VoxelMapData* startingMapData,
                                                         const f64v3& gridPosition,
                                                         const SoaState* soaState);
    extern void removeSphericalVoxelComponent(OUT SpaceSystem* spaceSystem, vcore::EntityID entity);
}

#endif // SpaceSystemFactories_h__