///
/// SpaceSystemAssemblages.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 13 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Component and entity assemblages for SpaceSystem
///

#pragma once

#ifndef SpaceSystemAssemblages_h__
#define SpaceSystemAssemblages_h__

class SpaceSystem;

#include "VoxelCoordinateSpaces.h"
#include "SpaceSystemLoadStructs.h"
#include "VoxPool.h"
#include <Vorb/VorbPreDecl.inl>
#include <Vorb/ecs/Entity.h>
#include <Vorb/graphics/gtypes.h>

struct PlanetGenData;
struct GasGiantProperties;
struct PlanetProperties;
struct SoaState;
struct SphericalTerrainComponent;
struct SphericalVoxelComponent;
struct StarProperties;
struct SystemBodyProperties;

DECL_VG(
    class GLProgram;
    class TextureRecycler;
)
DECL_VVOX(class VoxelMapData);

enum class SpaceBodyType;

namespace SpaceSystemAssemblages {
    /************************************************************************/
    /* Entity Factories                                                     */
    /************************************************************************/
   
    /// Planet entity
    extern vecs::EntityID createPlanet(SpaceSystem* spaceSystem,
                                       SystemBodyProperties* props);
    extern void destroyPlanet(SpaceSystem* gameSystem, vecs::EntityID planetEntity);

    /// Star entity
    extern vecs::EntityID createStar(SpaceSystem* spaceSystem,
                                     SystemBodyProperties* props);
    extern void destroyStar(SpaceSystem* gameSystem, vecs::EntityID planetEntity);

    /// GasGiant entity
    extern vecs::EntityID createGasGiant(SpaceSystem* spaceSystem,
                                         SystemBodyProperties* props);
    extern void destroyGasGiant(SpaceSystem* gameSystem, vecs::EntityID planetEntity);

    /************************************************************************/
    /* Component Assemblages                                                */
    /************************************************************************/
    /// PlanetRings component
    extern vecs::ComponentID addSpaceBodyComponent(SpaceSystem* spaceSystem, vecs::EntityID entity, const SystemBodyProperties* props);
    extern void removeSpaceBodyComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);


    /// Atmosphere component
    extern vecs::ComponentID addAtmosphereComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                    vecs::ComponentID namePositionComponent, f32 planetRadius,
                                                    f32 radius, f32 kr, f32 km, f32 g, f32 scaleDepth,
                                                    f32v3 wavelength, f32 oblateness = 0.0f);
    extern void removeAtmosphereComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);

    /// PlanetRings component
    extern vecs::ComponentID addPlanetRingsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                    vecs::ComponentID namePositionComponent, const Array<PlanetRingProperties>& rings);
    extern void removePlanetRingsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);

    /// Clouds component
    extern vecs::ComponentID addCloudsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                    vecs::ComponentID namePositionComponent, f32 planetRadius,
                                                    f32 height, f32v3 color, f32v3 scale, float density);
    extern void removeCloudsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);

    /// Spherical voxel component
    extern vecs::ComponentID addSphericalVoxelComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                         vecs::ComponentID sphericalTerrainComponent,
                                                         vecs::ComponentID farTerrainComponent,
                                                         vecs::ComponentID spaceBodyComponent,
                                                         WorldCubeFace worldFace,
                                                         SoaState* soaState);
    extern void removeSphericalVoxelComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);
    extern Event<SphericalVoxelComponent&, vecs::EntityID> onAddSphericalVoxelComponent;
    extern Event<SphericalVoxelComponent&, vecs::EntityID> onRemoveSphericalVoxelComponent;

    /// Star Component
    extern vecs::ComponentID addStarComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                              StarProperties& props);
    extern void removeStarComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);

    /// Gas giant component
    extern vecs::ComponentID addGasGiantComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                  GasGiantProperties& props);
    extern void removeGasGiantComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);

    /// Space Light Component
    extern vecs::ComponentID addSpaceLightComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                     vecs::ComponentID npCmp, color3 color, f32 intensity);
    extern void removeSpaceLightComponent(SpaceSystem* spaceSystem, vecs::EntityID entity);
}

#endif // SpaceSystemAssemblages_h__