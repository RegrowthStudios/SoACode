///
/// SpaceSystemLoader.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 4 Jun 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Loads the space system
///

#pragma once

#ifndef SpaceSystemLoader_h__
#define SpaceSystemLoader_h__

#include <Vorb/ecs/Entity.h>
#include <Vorb/VorbPreDecl.inl>
#include "VoxPool.h"
#include "SystemBodyLoader.h"

struct GasGiantProperties;
struct SystemBodyProperties;
struct SystemOrbitKegProperties;
class SpaceSystem;
class PlanetGenLoader;

DECL_VIO(class IOManager)
DECL_VCORE(class RPCManager)

class SpaceSystemLoader {
public:
    void init(const SoaState* soaState);
    /// Loads and adds a star system to the SpaceSystem
    /// @param pr: params
    void loadStarSystem(const nString& path);
private:
    /// Loads path color scheme
    /// @param pr: params
    /// @return true on success
    bool loadPathColors();

    /// Loads and adds system properties to the params
    /// @param pr: params
    /// @return true on success
    bool loadSystemProperties();

    /// Loads type specific properties
    bool loadSecondaryProperties(SystemBodyProperties* body);

    // Sets up mass parameters for binaries
    void initBarycenters();

    // Recursive function for binary creation
    void initBarycenter(SystemBodyProperties* bary);

    // Initializes orbits and parent connections
    void initOrbits();

    // Initializes entity component system
    void initComponents();

    void computeRef(SystemBodyProperties* body);

    void calculateOrbit(SystemBodyProperties* body,
                        f64 parentMass,
                        f64 binaryMassRatio = 0.0);

    SystemBodyLoader m_bodyLoader;
    
    const SoaState* m_soaState = nullptr;
    SpaceSystem* m_spaceSystem;
    vio::IOManager* m_ioManager = nullptr;
    vcore::ThreadPool<WorkerData>* m_threadpool = nullptr;
    std::map<nString, SystemBodyProperties*> m_barycenters;
    std::map<nString, SystemBodyProperties*> m_systemBodies;
   // std::map<nString, vecs::EntityID> m_bodyLookupMap;

    std::set<SystemBodyProperties*> m_calculatedOrbits;
};

#endif // SpaceSystemLoader_h__
