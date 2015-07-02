#include "stdafx.h"
#include "SphericalTerrainCpuGenerator.h"

#include "PlanetHeightData.h"
#include "VoxelSpaceConversions.h"
#include "CpuNoise.h"
#include "SimplexNoise.h"

//
//SphericalTerrainCpuGenerator::SphericalTerrainCpuGenerator(TerrainPatchMeshManager* meshManager,
//                                                           PlanetGenData* planetGenData) :
//    m_mesher(meshManager, planetGenData),
//    m_genData(planetGenData) {
//    // Empty
//}

void SphericalTerrainCpuGenerator::init(const PlanetGenData* planetGenData) {
    m_genData = planetGenData;
}

void SphericalTerrainCpuGenerator::generateHeight(OUT PlanetHeightData& height, const VoxelPosition2D& facePosition) const {
    // Need to convert to world-space
    f32v2 coordMults = f32v2(VoxelSpaceConversions::FACE_TO_WORLD_MULTS[(int)facePosition.face]);
    i32v3 coordMapping = VoxelSpaceConversions::VOXEL_TO_WORLD[(int)facePosition.face];

    f64v3 pos;
    pos[coordMapping.x] = facePosition.pos.x * KM_PER_VOXEL * coordMults.x;
    pos[coordMapping.y] = m_genData->radius * (f64)VoxelSpaceConversions::FACE_Y_MULTS[(int)facePosition.face];
    pos[coordMapping.z] = facePosition.pos.y * KM_PER_VOXEL * coordMults.y;

    pos = glm::normalize(pos) * m_genData->radius;

    height.height = (m_genData->baseTerrainFuncs.base + getNoiseValue(pos, m_genData->baseTerrainFuncs.funcs, nullptr, TerrainOp::ADD)) * VOXELS_PER_M;
    height.temperature = m_genData->tempTerrainFuncs.base + getNoiseValue(pos, m_genData->tempTerrainFuncs.funcs, nullptr, TerrainOp::ADD);
    height.rainfall = m_genData->humTerrainFuncs.base + getNoiseValue(pos, m_genData->humTerrainFuncs.funcs, nullptr, TerrainOp::ADD);
    height.surfaceBlock = m_genData->surfaceBlock; // TODO(Ben): Naw dis is bad mkay
}

f64 SphericalTerrainCpuGenerator::getHeight(const VoxelPosition2D& facePosition) const {
    // Need to convert to world-space
    f32v2 coordMults = f32v2(VoxelSpaceConversions::FACE_TO_WORLD_MULTS[(int)facePosition.face]);
    i32v3 coordMapping = VoxelSpaceConversions::VOXEL_TO_WORLD[(int)facePosition.face];

    f64v3 pos;
    pos[coordMapping.x] = facePosition.pos.x * M_PER_VOXEL * coordMults.x;
    pos[coordMapping.y] = m_genData->radius * (f64)VoxelSpaceConversions::FACE_Y_MULTS[(int)facePosition.face];
    pos[coordMapping.z] = facePosition.pos.y * M_PER_VOXEL * coordMults.y;

    pos = glm::normalize(pos) * m_genData->radius;
    return m_genData->baseTerrainFuncs.base + getNoiseValue(pos, m_genData->baseTerrainFuncs.funcs, nullptr, TerrainOp::ADD);
}

f64 SphericalTerrainCpuGenerator::getHeightValue(const f64v3& pos) const {
    return m_genData->baseTerrainFuncs.base + getNoiseValue(pos, m_genData->baseTerrainFuncs.funcs, nullptr, TerrainOp::ADD);
}

f64 SphericalTerrainCpuGenerator::getTemperatureValue(const f64v3& pos) const {
    return m_genData->tempTerrainFuncs.base + getNoiseValue(pos, m_genData->tempTerrainFuncs.funcs, nullptr, TerrainOp::ADD);
}

f64 SphericalTerrainCpuGenerator::getHumidityValue(const f64v3& pos) const {
    return m_genData->humTerrainFuncs.base + getNoiseValue(pos, m_genData->humTerrainFuncs.funcs, nullptr, TerrainOp::ADD);
}

f64 doOperation(const TerrainOp& op, f64 a, f64 b) {
    switch (op) {
        case TerrainOp::ADD: return a + b;
        case TerrainOp::SUB: return a - b;
        case TerrainOp::MUL: return a * b;
        case TerrainOp::DIV: return a / b;
    }
    return 0.0f;
}

f64 SphericalTerrainCpuGenerator::getNoiseValue(const f64v3& pos,
                                                const Array<TerrainFuncKegProperties>& funcs,
                                                f64* modifier,
                                                const TerrainOp& op) const {

    f64 rv = 0.0f;
    f64* nextMod;

    TerrainOp nextOp;

    // NOTE: Make sure this implementation matches NoiseShaderGenerator::addNoiseFunctions()
    for (size_t f = 0; f < funcs.size(); f++) {
        auto& fn = funcs[f];

        f64 h = 0.0f;

        // Check if its not a noise function
        if (fn.func == TerrainStage::CONSTANT) {
            nextMod = &h;
            h = fn.low;
            // Apply parent before clamping
            if (modifier) {
                h = doOperation(op, h, *modifier);
            }
            // Optional clamp if both fields are not 0.0f
            if (fn.clamp[0] != 0.0f || fn.clamp[1] != 0.0f) {
                h = glm::clamp(*modifier, (f64)fn.clamp[0], (f64)fn.clamp[1]);
            }
            nextOp = fn.op;
        } else if (fn.func == TerrainStage::PASS_THROUGH) {
            nextMod = modifier;
            // Apply parent before clamping
            if (modifier) {
                h = doOperation(op, *modifier, fn.low);
                // Optional clamp if both fields are not 0.0f
                if (fn.clamp[0] != 0.0f || fn.clamp[1] != 0.0f) {
                    h = glm::clamp(h, (f64)fn.clamp[0], (f64)fn.clamp[1]);
                }
            }
            nextOp = op;
        } else if (fn.func == TerrainStage::SQUARED) {
            nextMod = modifier;
            // Apply parent before clamping
            if (modifier) {
                *modifier = (*modifier) * (*modifier);
                // Optional clamp if both fields are not 0.0f
                if (fn.clamp[0] != 0.0f || fn.clamp[1] != 0.0f) {
                    h = glm::clamp(h, (f64)fn.clamp[0], (f64)fn.clamp[1]);
                }
            }
            nextOp = op;
        } else if (fn.func == TerrainStage::CUBED) {
            nextMod = modifier;
            // Apply parent before clamping
            if (modifier) {
                *modifier = (*modifier) * (*modifier) * (*modifier);
                // Optional clamp if both fields are not 0.0f
                if (fn.clamp[0] != 0.0f || fn.clamp[1] != 0.0f) {
                    h = glm::clamp(h, (f64)fn.clamp[0], (f64)fn.clamp[1]);
                }
            }
            nextOp = op;
        } else { // It's a noise function
            nextMod = &h;
            f64v2 ff;
            f64 tmp;
            f64 total = 0.0;
            f64 maxAmplitude = 0.0;
            f64 amplitude = 1.0;
            f64 frequency = fn.frequency;
            for (int i = 0; i < fn.octaves; i++) {
                
                switch (fn.func) {
                    case TerrainStage::CUBED_NOISE:
                    case TerrainStage::SQUARED_NOISE:
                    case TerrainStage::NOISE:
                        total += raw_noise_3d(pos.x * frequency, pos.y * frequency, pos.z * frequency) * amplitude;
                        break;
                    case TerrainStage::RIDGED_NOISE:
                        total += ((1.0 - glm::abs(raw_noise_3d(pos.x * frequency, pos.y * frequency, pos.z * frequency))) * 2.0 - 1.0) * amplitude;
                        break;
                    case TerrainStage::ABS_NOISE:
                        total += glm::abs(raw_noise_3d(pos.x * frequency, pos.y * frequency, pos.z * frequency)) * amplitude;
                        break;
                    case TerrainStage::CELLULAR_NOISE:
                        ff = CpuNoise::cellular(pos * (f64)frequency);
                        total += (ff.y - ff.x) * amplitude;
                        break;
                    case TerrainStage::CELLULAR_SQUARED_NOISE:
                        ff = CpuNoise::cellular(pos * (f64)frequency);
                        tmp = ff.y - ff.x;
                        total += tmp * tmp * amplitude;
                        break;
                    case TerrainStage::CELLULAR_CUBED_NOISE:
                        ff = CpuNoise::cellular(pos * (f64)frequency);
                        tmp = ff.y - ff.x;
                        total += tmp * tmp * tmp * amplitude;
                        break;
                }
                frequency *= 2.0;
                maxAmplitude += amplitude;
                amplitude *= fn.persistence;
            }
            total = (total / maxAmplitude);
            // Handle any post processes per noise
            switch (fn.func) {
                case TerrainStage::CUBED_NOISE:
                    total = total * total * total;
                    break;
                case TerrainStage::SQUARED_NOISE:
                    total = total * total;
                    break;
                default:
                    break;
            }
            // Conditional scaling. 
            if (fn.low != -1.0f || fn.high != 1.0f) {
                h = total * (fn.high - fn.low) * 0.5 + (fn.high + fn.low) * 0.5;
            } else {
                h = total;
            }
            // Optional clamp if both fields are not 0.0f
            if (fn.clamp[0] != 0.0f || fn.clamp[1] != 0.0f) {
                h = glm::clamp(h, (f64)fn.clamp[0], (f64)fn.clamp[1]);
            }
            // Apply modifier from parent if needed
            if (modifier) {
                h = doOperation(op, h, *modifier);
            }
            nextOp = fn.op;
        }

        if (fn.children.size()) {
            getNoiseValue(pos, fn.children, nextMod, nextOp);
            rv += h;
        } else {
            rv = doOperation(fn.op, rv, h);
        }
    }
    return rv;
}
