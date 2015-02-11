#include "stdafx.h"
#include "SphericalTerrainCpuGenerator.h"

#include "PlanetData.h"
#include "VoxelSpaceConversions.h"
#include "CpuNoise.h"

#define KM_PER_VOXEL 0.0005f

SphericalTerrainCpuGenerator::SphericalTerrainCpuGenerator(SphericalTerrainMeshManager* meshManager,
                                                           PlanetGenData* planetGenData) :
    m_mesher(meshManager, planetGenData),
    m_genData(planetGenData) {
    // Empty
}


SphericalTerrainCpuGenerator::~SphericalTerrainCpuGenerator() {
    // Empty
}

void SphericalTerrainCpuGenerator::generateTerrainPatch(OUT SphericalTerrainMesh* mesh, const f32v3& startPos, WorldCubeFace cubeFace, float width) {

    f32v3 pos;
    const i32v3& coordMapping = VoxelSpaceConversions::GRID_TO_WORLD[(int)cubeFace];
    const f32v2& coordMults = f32v2(VoxelSpaceConversions::FACE_TO_WORLD_MULTS[(int)cubeFace][0]);
    
    const float VERT_WIDTH = width / PATCH_WIDTH;

    int zIndex;
    int xIndex;
    // TODO(Ben): If we want to do MT CPU gen we cant use static buffers
    static float heightData[PATCH_HEIGHTMAP_WIDTH][PATCH_HEIGHTMAP_WIDTH][4];
    memset(heightData, 0, sizeof(heightData));
    for (int z = 0; z < PATCH_WIDTH; z++) {
        for (int x = 0; x < PATCH_WIDTH; x++) {

            pos[coordMapping.x] = startPos.x + x * VERT_WIDTH * coordMults.x;
            pos[coordMapping.y] = startPos.y;
            pos[coordMapping.z] = startPos.z + z * VERT_WIDTH * coordMults.y;

            zIndex = z * PIXELS_PER_PATCH_NM + 1;
            xIndex = x * PIXELS_PER_PATCH_NM + 1;

            heightData[zIndex][xIndex][0] = getNoiseValue(pos, m_genData->baseTerrainFuncs);
            heightData[zIndex][xIndex][1] = getNoiseValue(pos, m_genData->tempTerrainFuncs);
            heightData[zIndex][xIndex][2] = getNoiseValue(pos, m_genData->humTerrainFuncs);
            // TODO(Ben): Biome
         //   heightData[zIndex][xIndex][3] = ;
        }
    }

    m_mesher.buildMesh(mesh, startPos, cubeFace, width, heightData);
}

float SphericalTerrainCpuGenerator::getTerrainHeight(const VoxelFacePosition2D& facePosition) {
    // Get scaled position
    f32v2 coordMults = f32v2(VoxelSpaceConversions::FACE_TO_WORLD_MULTS[(int)facePosition.face][0]);

    // Set the data
    f32v3 pos(facePosition.pos.x * CHUNK_WIDTH * KM_PER_VOXEL * coordMults.x,
              m_genData->radius * VoxelSpaceConversions::FACE_Y_MULTS[(int)facePosition.face],
              facePosition.pos.y * CHUNK_WIDTH * KM_PER_VOXEL * coordMults.y);

    return getNoiseValue(pos, m_genData->baseTerrainFuncs);
}

float SphericalTerrainCpuGenerator::getNoiseValue(const f32v3& pos, const TerrainFuncs& funcs) {

#define SCALE_CODE rv += (total / maxAmplitude) * (fn.high - fn.low) * 0.5f + (fn.high + fn.low) * 0.5f

    float rv = funcs.baseHeight;
    float total;
    float amplitude;
    float maxAmplitude;
    float frequency;

    // NOTE: Make sure this implementation matches NoiseShaderGenerator::addNoiseFunctions()
    for (auto& fn : funcs.funcs) {
        switch (fn.func) {
            case TerrainFunction::NOISE:
                total = 0.0f;
                amplitude = 1.0f;
                maxAmplitude = 0.0f;
                frequency = fn.frequency;
                for (int i = 0; i < fn.octaves; i++) {
                    total += CpuNoise::rawAshimaSimplex3D(pos * frequency) * amplitude;

                    frequency *= 2.0f;
                    maxAmplitude += amplitude;
                    amplitude *= fn.persistence;
                }
                SCALE_CODE;
                break;
            case TerrainFunction::RIDGED_NOISE:
                total = 0.0f;
                amplitude = 1.0f;
                maxAmplitude = 0.0f;
                frequency = fn.frequency;
                for (int i = 0; i < fn.octaves; i++) {
                    total += ((1.0f - glm::abs(CpuNoise::rawAshimaSimplex3D(pos * frequency))) * 2.0f - 1.0f) * amplitude;

                    frequency *= 2.0f;
                    maxAmplitude += amplitude;
                    amplitude *= fn.persistence;
                }
                SCALE_CODE;
                break;
            default:
                break;
        }
    }
    return rv;
}