#include "stdafx.h"
#include "SpaceSystemAssemblages.h"

#include "ChunkGrid.h"
#include "ChunkIOManager.h"
#include "ChunkAllocator.h"
#include "FarTerrainPatch.h"
#include "SpaceBodyComponentUpdater.h"
#include "SoaState.h"
#include "SpaceSystem.h"
#include "SphericalTerrainComponentUpdater.h"
#include "SphericalHeightmapGenerator.h"

#include "TerrainPatchMeshManager.h"
#include "SpaceSystemAssemblages.h"
#include "SpaceSystemLoadStructs.h"

// TEMPORARY
#include "GameManager.h"
#include "PlanetGenData.h"

#define SEC_PER_HOUR 3600.0

Event<SphericalVoxelComponent&, vecs::EntityID> SpaceSystemAssemblages::onAddSphericalVoxelComponent;
Event<SphericalVoxelComponent&, vecs::EntityID> SpaceSystemAssemblages::onRemoveSphericalVoxelComponent;

vecs::EntityID SpaceSystemAssemblages::createPlanet(SpaceSystem* spaceSystem,
                                                    SystemBodyProperties* props) {
    props->entity = spaceSystem->addEntity();
    const vecs::EntityID& id = props->entity;

    const f64v3 up(0.0, 1.0, 0.0);
    vecs::ComponentID cmp = addSpaceBodyComponent(spaceSystem, id, props);

    return id;
}

void SpaceSystemAssemblages::destroyPlanet(SpaceSystem* gameSystem, vecs::EntityID planetEntity) {
    // TODO: implement
}

vecs::EntityID SpaceSystemAssemblages::createStar(SpaceSystem* spaceSystem,
                                                  SystemBodyProperties* props) {
    props->entity = spaceSystem->addEntity();
    const vecs::EntityID& id = props->entity;

    const f64v3 up(0.0, 1.0, 0.0);
    vecs::ComponentID cmp = addSpaceBodyComponent(spaceSystem, id, props);

    addStarComponent(spaceSystem, id, props->starProperties);

    return id;
}

void SpaceSystemAssemblages::destroyStar(SpaceSystem* gameSystem, vecs::EntityID planetEntity) {
    // TODO: implement
}

/// GasGiant entity
vecs::EntityID SpaceSystemAssemblages::createGasGiant(SpaceSystem* spaceSystem,
                                                      SystemBodyProperties* props) {
    props->entity = spaceSystem->addEntity();
    const vecs::EntityID& id = props->entity;

    const f64v3 up(0.0, 1.0, 0.0);
    vecs::ComponentID cmp = addSpaceBodyComponent(spaceSystem, id, props);

    addGasGiantComponent(spaceSystem, id, props->gasGiantProperties);

    return id;
}

void SpaceSystemAssemblages::destroyGasGiant(SpaceSystem* gameSystem, vecs::EntityID planetEntity) {
    // TODO: implement
}


vecs::ComponentID SpaceSystemAssemblages::addSpaceBodyComponent(SpaceSystem* spaceSystem, vecs::EntityID entity, const SystemBodyProperties* props) {
    vecs::ComponentID cmpID = spaceSystem->addComponent(SPACE_SYSTEM_CT_SPACEBODY_NAME, entity);
    SpaceBodyComponent& cmp = spaceSystem->spaceBody.get(cmpID);

    // TODO(Ben): Initialize component
}

void SpaceSystemAssemblages::removeSpaceBodyComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_SPACEBODY_NAME, entity);
}

vecs::ComponentID SpaceSystemAssemblages::addAtmosphereComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                                 vecs::ComponentID namePositionComponent, f32 planetRadius,
                                                                 f32 radius, f32 kr, f32 km, f32 g, f32 scaleDepth,
                                                                 f32v3 wavelength, f32 oblateness /*= 0.0f*/) {
    vecs::ComponentID aCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_ATMOSPHERE_NAME, entity);
    auto& aCmp = spaceSystem->atmosphere.get(aCmpId);
    aCmp.bodyComponent = namePositionComponent;
    aCmp.planetRadius = planetRadius;
    aCmp.radius = radius;
    aCmp.oblateness = oblateness;
    aCmp.kr = kr;
    aCmp.km = km;
    aCmp.g = g;
    aCmp.scaleDepth = scaleDepth;
    aCmp.invWavelength4 = f32v3(1.0f / powf(wavelength.r, 4.0f),
                                1.0f / powf(wavelength.g, 4.0f),
                                1.0f / powf(wavelength.b, 4.0f));
    return aCmpId;
}

void SpaceSystemAssemblages::removeAtmosphereComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_ATMOSPHERE_NAME, entity);
}

vecs::ComponentID SpaceSystemAssemblages::addPlanetRingsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                          vecs::ComponentID namePositionComponent, const Array<PlanetRingProperties>& rings) {
    vecs::ComponentID prCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_PLANETRINGS_NAME, entity);
    if (prCmpId == 0) {
        return 0;
    }
    auto& prCmp = spaceSystem->planetRings.get(prCmpId);
    prCmp.bodyComponent = namePositionComponent;
    prCmp.rings.resize(rings.size());
    for (size_t i = 0; i < rings.size(); i++) {
        auto& r1 = prCmp.rings[i];
        auto& r2 = rings[i];
        r1.innerRadius = r2.innerRadius;
        r1.outerRadius = r2.outerRadius;
        r1.texturePath = r2.colorLookup;
        r1.orientation = vmath::angleAxis((f64)r2.lNorth, f64v3(0.0, 1.0, 0.0)) * vmath::angleAxis((f64)r2.aTilt, f64v3(1.0, 0.0, 0.0));
    }
    return prCmpId;
}

void  SpaceSystemAssemblages::removePlanetRingsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_PLANETRINGS_NAME, entity);
}

vecs::ComponentID SpaceSystemAssemblages::addCloudsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                            vecs::ComponentID namePositionComponent, f32 planetRadius,
                                                            f32 height, f32v3 color, f32v3 scale, float density) {

    vecs::ComponentID cCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_CLOUDS_NAME, entity);
    auto& cCmp = spaceSystem->clouds.get(cCmpId);
    cCmp.bodyComponent = namePositionComponent;
    cCmp.planetRadius = planetRadius;
    cCmp.height = height;
    cCmp.color = color;
    cCmp.scale = scale;
    cCmp.density = density;
    return cCmpId;
}

void SpaceSystemAssemblages::removeCloudsComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_CLOUDS_NAME, entity);
}

vecs::ComponentID SpaceSystemAssemblages::addSphericalVoxelComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                                      vecs::ComponentID sphericalTerrainComponent,
                                                                      vecs::ComponentID farTerrainComponent,
                                                                      vecs::ComponentID spaceBodyComponent,
                                                                      WorldCubeFace worldFace,
                                                                      SoaState* soaState) {

    //vecs::ComponentID svCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_SPHERICALVOXEL_NAME, entity);
    //if (svCmpId == 0) {
    //    return 0;
    //}
    //auto& svcmp = spaceSystem->sphericalVoxel.get(svCmpId);

    //auto& ftcmp = spaceSystem->farTerrain.get(farTerrainComponent);

    //// Get component handles
    //svcmp.sphericalTerrainComponent = sphericalTerrainComponent;
    //svcmp.bodyComponent = spaceBodyComponent;
    //svcmp.farTerrainComponent = farTerrainComponent;

    //svcmp.voxelRadius = ftcmp.sphericalTerrainData->radius * VOXELS_PER_KM;

    //svcmp.generator = ftcmp.cpuGenerator;
    //svcmp.chunkIo = new ChunkIOManager("TESTSAVEDIR"); // TODO(Ben): Fix
    //svcmp.blockPack = &soaState->blocks;

    //svcmp.threadPool = soaState->threadPool;

    //svcmp.chunkIo->beginThread();

    //svcmp.chunkGrids = new ChunkGrid[6];
    //for (int i = 0; i < 6; i++) {
    //    svcmp.chunkGrids[i].init(static_cast<WorldCubeFace>(i), svcmp.threadPool, 1, ftcmp.planetGenData, &soaState->chunkAllocator);
    //    svcmp.chunkGrids[i].blockPack = &soaState->blocks;
    //}

    //svcmp.planetGenData = ftcmp.planetGenData;
    //svcmp.sphericalTerrainData = ftcmp.sphericalTerrainData;
    //svcmp.saveFileIom = &soaState->saveFileIom;

    //onAddSphericalVoxelComponent(svcmp, entity);
    //return svCmpId;
}

void SpaceSystemAssemblages::removeSphericalVoxelComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    onRemoveSphericalVoxelComponent(spaceSystem->sphericalVoxel.getFromEntity(entity), entity);
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_SPHERICALVOXEL_NAME, entity);
}

/// Star Component
vecs::ComponentID SpaceSystemAssemblages::addStarComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                           StarProperties& props) {
    vecs::ComponentID sCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_STAR_NAME, entity);
    auto& sCmp = spaceSystem->star.get(sCmpId);

    // TODO: Implement
    throw 33;

    return sCmpId;
}
void SpaceSystemAssemblages::removeStarComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_STAR_NAME, entity);
}

// Gas Giant Component
vecs::ComponentID SpaceSystemAssemblages::addGasGiantComponent(SpaceSystem* spaceSystem, vecs::EntityID entity,
                                                               GasGiantProperties& props) {
    vecs::ComponentID sCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_GASGIANT_NAME, entity);
    auto& sCmp = spaceSystem->star.get(sCmpId);

    // TODO: Implement
    throw 33;

    return sCmpId;
}
void SpaceSystemAssemblages::removeGasGiantComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_GASGIANT_NAME, entity);
}

vecs::ComponentID SpaceSystemAssemblages::addSpaceLightComponent(SpaceSystem* spaceSystem, vecs::EntityID entity, vecs::ComponentID npCmp, color3 color, f32 intensity) {
    vecs::ComponentID slCmpId = spaceSystem->addComponent(SPACE_SYSTEM_CT_SPACELIGHT_NAME, entity);
    auto& slCmp = spaceSystem->spaceLight.get(slCmpId);
    slCmp.color = color;
    slCmp.intensity = intensity;
    slCmp.bodyComponent = npCmp;
    return slCmpId;
}

void SpaceSystemAssemblages::removeSpaceLightComponent(SpaceSystem* spaceSystem, vecs::EntityID entity) {
    spaceSystem->deleteComponent(SPACE_SYSTEM_CT_SPACELIGHT_NAME, entity);
}
