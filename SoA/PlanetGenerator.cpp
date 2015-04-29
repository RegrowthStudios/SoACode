#include "stdafx.h"
#include "PlanetGenerator.h"
#include "PlanetData.h"

#include <Vorb/graphics/GpuMemory.h>
#include <Vorb/graphics/SamplerState.h>
#include <random>

CALLEE_DELETE PlanetGenData* PlanetGenerator::generateRandomPlanet(ObjectType type) {
    switch (type) {
        case ObjectType::PLANET:
        case ObjectType::DWARF_PLANET:
        case ObjectType::MOON:
        case ObjectType::DWARF_MOON:
            return generatePlanet();
        case ObjectType::ASTEROID:
            return generateAsteroid();
        case ObjectType::COMET:
            return generateComet();
        default:
            return nullptr;
    }
}

CALLEE_DELETE PlanetGenData* PlanetGenerator::generatePlanet() {
    PlanetGenData* data = new PlanetGenData;
    data->terrainColorMap = getRandomColorMap();
    data->terrainTexture = getRandomColorMap();
    data->liquidColorMap = getRandomColorMap();

    TerrainFuncKegProperties tf1;
    tf1.low = 1.0;
    tf1.high = 100.0;
    data->baseTerrainFuncs.funcs.setData(&tf1, 1);
    data->tempTerrainFuncs.funcs.setData(&tf1, 1);
    data->humTerrainFuncs.funcs.setData(&tf1, 1);
    return data;
}

CALLEE_DELETE PlanetGenData* PlanetGenerator::generateAsteroid() {
    PlanetGenData* data = new PlanetGenData;
    return data;
}

CALLEE_DELETE PlanetGenData* PlanetGenerator::generateComet() {
    PlanetGenData* data = new PlanetGenData;
    return data;
}

VGTexture PlanetGenerator::getRandomColorMap() {
    static std::mt19937 generator(1053);
    static std::uniform_int_distribution<int> colors(0, 6);

    int r = colors(generator);
    ui8v4 color;

    switch (r) {
        case 0:
            color = ui8v4(255, 0, 0, 255); break;
        case 1:
            color = ui8v4(0, 255, 0, 255); break;
        case 2:
            color = ui8v4(0, 0, 255, 255); break;
        case 3:
            color = ui8v4(255, 255, 0, 255); break;
        case 4:
            color = ui8v4(0, 255, 255, 255); break;
        case 5:
            color = ui8v4(255, 255, 255, 255); break;
        case 6:
            color = ui8v4(128, 128, 128, 255); break;
        default:
            break;
    }

    return vg::GpuMemory::uploadTexture(&color[0], 1, 1, &vg::SamplerState::POINT_CLAMP);
}