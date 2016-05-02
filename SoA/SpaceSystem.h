///
/// SpaceSystem.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 8 Dec 2014
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Header for the Space System module.
/// Implementation of a Star System with ECS.
///

#pragma once

#ifndef SpaceSystem_h__
#define SpaceSystem_h__

#include "SpaceSystemComponents.h"
#include "SpaceSystemComponentTables.h"

#include <Vorb/io/IOManager.h>
#include <Vorb/ecs/ComponentTable.hpp>
#include <Vorb/ecs/ECS.h>
#include <Vorb/VorbPreDecl.inl>
#include <Vorb/graphics/GLProgram.h>

// TODO(Ben): Integers?
#define SPACE_SYSTEM_CT_SPACEBODY_NAME "SpaceBody"
#define SPACE_SYSTEM_CT_SPHERICALTERRAIN_NAME "SphericalTerrain"
#define SPACE_SYSTEM_CT_FARTERRAIN_NAME "FarTerrain"
#define SPACE_SYSTEM_CT_GASGIANT_NAME "GasGiant"
#define SPACE_SYSTEM_CT_STAR_NAME "Star"
#define SPACE_SYSTEM_CT_SPHERICALVOXEL_NAME "SphericalVoxel"
#define SPACE_SYSTEM_CT_SPACELIGHT_NAME "SpaceLight"
#define SPACE_SYSTEM_CT_ATMOSPHERE_NAME "Atmosphere"
#define SPACE_SYSTEM_CT_PLANETRINGS_NAME "PlanetRings"
#define SPACE_SYSTEM_CT_CLOUDS_NAME "Clouds"

class App;
class Binary;
class Camera;
class GameSystem;
class PlanetGenLoader;
struct SoaState;
class SpriteBatch;
class SpriteFont;
struct GasGiantProperties;
struct PlanetProperties;
struct StarProperties;
struct SystemBodyProperties;
struct SystemOrbitKegProperties;

DECL_VG(class TextureRecycler)

enum class SystemState { NONE, PROPERTIES_LOADED, };

class SpaceSystem : public vecs::ECS {
    friend class SpaceSystemLoader;
    friend class SpaceSystemRenderStage;
    friend class MainMenuSystemViewer;
public:
    SpaceSystem();
    ~SpaceSystem();

    /************************************************************************/
    /* Component Tables                                                     */
    /************************************************************************/
    SpaceBodyComponentTable spaceBody;
    SphericalTerrainComponentTable sphericalTerrain;
    GasGiantComponentTable gasGiant;
    StarComponentTable star;
    FarTerrainComponentTable farTerrain;
    SpaceLightComponentTable spaceLight;
    AtmosphereComponentTable atmosphere;
    PlanetRingsComponentTable planetRings;
    CloudsComponentTable clouds;
    SphericalVoxelComponentTable sphericalVoxel;
    /************************************************************************/

    vecs::ComponentID getComponent(const nString& name, vecs::EntityID eID);

private:
    f32 m_age = 0.0f; ///< age of the system
    nString m_systemDescription = "No description"; ///< textual description of the system

    VORB_NON_COPYABLE(SpaceSystem);
};

#endif // SpaceSystem_h__
