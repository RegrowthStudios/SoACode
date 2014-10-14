#include "stdafx.h"

#include "BlockData.h"
#include "ChunkGenerator.h"
#include "Chunk.h"
#include "WorldStructs.h"
#include "VoxelIntervalTree.h"
#include "TerrainGenerator.h"
#include "Planet.h"
#include "GameManager.h"

#include "Timing.h"

bool ChunkGenerator::generateChunk(Chunk* chunk, struct LoadData *ld)
{
    PreciseTimer timer;
    timer.start();
    HeightData *heightMap = ld->heightMap;

    // used for tree coords
    int wz, wx;
    chunk->voxelMapData->getVoxelGridPos(wz, wx);

    TerrainGenerator &generator = *(ld->generator);
    Biome *biome;
    chunk->numBlocks = 0;
    int temperature;
    int rainfall;
    double CaveDensity1[9][5][5], CaveDensity2[9][5][5];

    std::vector<VoxelIntervalTree<ui16>::LightweightNode> dataVector;
    std::vector<VoxelIntervalTree<ui16>::LightweightNode> lampVector;
    std::vector<VoxelIntervalTree<ui8>::LightweightNode> sunVector;

    chunk->numNeighbors = 0;

    int c = 0;
    int flags;
    int h = 0, maph;
    int hindex;
    int snowDepth, sandDepth;
    double r;
    int needsCave = 3;
    bool needsSurface = 1;
    int pnum;
    int nh;
    double ti, tj;
    bool tooSteep;

    chunk->minh = chunk->gridPosition.y - heightMap[CHUNK_LAYER / 2].height; //pick center height as the overall height for minerals

    ui16 data;
    ui8 lampData;
    ui8 sunlightData;

    for (int y = 0; y < CHUNK_WIDTH; y++){
        pnum = chunk->numBlocks;
        for (int z = 0; z < CHUNK_WIDTH; z++){
            for (int x = 0; x < CHUNK_WIDTH; x++, c++){

                hindex = (c%CHUNK_LAYER);

                data = 0;
                sunlightData = 0;
                lampData = 0;

                snowDepth = heightMap[hindex].snowDepth;
                sandDepth = heightMap[hindex].sandDepth;
                maph = heightMap[hindex].height;
                biome = heightMap[hindex].biome;
                temperature = heightMap[hindex].temperature;
                rainfall = heightMap[hindex].rainfall;
                flags = heightMap[hindex].flags;

                tooSteep = (flags & TOOSTEEP) != 0;

                h = y +  chunk->gridPosition.y;
                nh = (maph - 1) - h;

                if ((h <= maph - 1) && (!tooSteep || maph - h > (biome->looseSoilDepth - 1))){ //ROCK LAYERS check for loose soil too
                    if ((h - (maph - 1)) > -SURFACE_DEPTH){ //SURFACE LAYERS
                        if (nh >= SURFACE_DEPTH) exit(1);
                        data = biome->surfaceLayers[nh];
                        chunk->numBlocks++;
                    } else{ //VERY LOW
                        data = STONE;
                        chunk->numBlocks++;
                    }
                } else if (h == maph && !tooSteep){ //surface
                    data = heightMap[hindex].surfaceBlock;
                    chunk->numBlocks++;
                    if (!sandDepth && biome->beachBlock != SAND){
                        if (snowDepth < 7){
                            TryEnqueueTree(chunk, biome, x + wx, z + wz, c);
                        }
                    }
                    if (!sandDepth && (h > 0 || h == 0 && data != SAND)){
                        if (snowDepth < 7){
                            TryEnqueueTree(chunk, biome, x + wx, z + wz, c);
                        }
                    }
                } else if (sandDepth && h == maph + sandDepth){ //tree layers
                    data = SAND;
                    chunk->numBlocks++;
                    if (snowDepth < 7 && h > 0){
                        TryEnqueueTree(chunk, biome, x + wx, z + wz, c);
                    }
                } else if (sandDepth && h - maph <= sandDepth){
                    data = SAND;
                    chunk->numBlocks++;
                } else if (snowDepth && h - maph <= sandDepth + snowDepth){
                    data = SNOW;
                    chunk->numBlocks++;
                } else if (h < 0){ //

                    if (temperature < h + FREEZETEMP){ //underwater glacial mass
                        data = ICE;
                    } else{
                        data = FULLWATER;

                    }
                    chunk->numBlocks++;
                } else if (h == 0 && maph < h){ //shoreline
                    if (temperature < FREEZETEMP){
                        data = ICE;
                    } else{
                        data = FULLWATER - 60;
                    }
                    chunk->numBlocks++;
                } else if (h == maph + 1 && h > 0){ //FLORA!

                    if (biome->possibleFlora.size()){
                        r = chunk->GetPlantType(x + wx, z + wz, biome);
                        if (r) chunk->plantsToLoad.emplace_back(GameManager::planet->floraTypeVec[r], c);

                         sunlightData = MAXLIGHT;
                         data = NONE;

                         chunk->sunExtendList.push_back(c);

                    } else{
                         sunlightData = MAXLIGHT;
                         data = NONE;

                       
                         chunk->sunExtendList.push_back(c);
                    }
                } else if (h == maph + 1)
                {
                     sunlightData = MAXLIGHT;
                     data = NONE;

                     chunk->sunExtendList.push_back(c);
                    
                } else if (maph < 0 && temperature < FREEZETEMP && h < (FREEZETEMP - temperature)){ //above ground glacier activity
                    data = ICE;
                    chunk->numBlocks++;
                } else{
                    sunlightData = MAXLIGHT;
                    data = NONE;
                    if (h == 1){
                         chunk->sunExtendList.push_back(c);
                    }
                }

                nh = h - (maph + snowDepth + sandDepth);
                if (maph < 0) nh += 6; //add six layers of rock when underground to prevent water walls
                if (data != NONE && (GETBLOCKTYPE(data) < LOWWATER) && nh <= 1){

                    if (needsCave == 3){
                        generator.CalculateCaveDensity( chunk->gridPosition, (double *)CaveDensity1, 9, 0, 5, 0.6, 0.0004);
                        needsCave = 2;
                    }
                    ti = trilinearInterpolation_4_8_4(x, y, z, CaveDensity1);
                    if (ti > 0.905 && ti < 0.925){
                        if (needsCave == 2){
                            generator.CalculateCaveDensity( chunk->gridPosition, (double *)CaveDensity2, 9, 8000, 4, 0.67, 0.0004);
                            needsCave = 1;
                        }
                        tj = trilinearInterpolation_4_8_4(x, y, z, CaveDensity2);

                        if (tj > 0.9 && tj < 0.941){

                            //frozen caves
                            if (temperature <FREEZETEMP && (ti < 0.908 || ti > 0.922 || tj < 0.903 || tj > 0.938)){
                                data = ICE;
                            } else{
                                data = NONE;
                                chunk->numBlocks--;
                            }

                        }
                    }
                }
                if (GETBLOCK(data).spawnerVal || GETBLOCK(data).sinkVal){
                    chunk->spawnerBlocks.push_back(c); 
                }

                //modify the data
                if (c == 0) {

                    dataVector.emplace_back(0, 1, data);
                    lampVector.emplace_back(0, 1, lampData);
                    sunVector.emplace_back(0, 1, sunlightData);

                } else {

                    if (data == dataVector.back().data) {
                        dataVector.back().length++;
                    } else {
                        dataVector.emplace_back(c, 1, data);
                    }
                    if (lampData == lampVector.back().data) {
                        lampVector.back().length++;
                    } else {
                        lampVector.emplace_back(c, 1, lampData);
                    }
                    if (sunlightData == sunVector.back().data) {
                        sunVector.back().length++;
                    } else {
                        sunVector.emplace_back(c, 1, sunlightData);
                    }
                   
                }
            }
        }
        if (pnum == chunk->numBlocks && maph - h < 0){

            if (dataVector.back().data == 0) {
                dataVector.back().length += CHUNK_SIZE - c;
            } else {
                dataVector.emplace_back(c, CHUNK_SIZE - c, 0);
            }
            if (lampVector.back().data == 0) {
                lampVector.back().length += CHUNK_SIZE - c;
            } else {
                lampVector.emplace_back(c, CHUNK_SIZE - c, 0);
            }
            if (sunVector.back().data == MAXLIGHT) {
                sunVector.back().length += CHUNK_SIZE - c;
            } else {
                sunVector.emplace_back(c, CHUNK_SIZE - c, MAXLIGHT);
            }
            break;
        }
    }

    chunk->_blockIDContainer.initFromSortedArray(dataVector);
    chunk->_lampLightContainer.initFromSortedArray(lampVector);
    chunk->_sunlightContainer.initFromSortedArray(sunVector);

    if (chunk->numBlocks){
        LoadMinerals(chunk);
    }

    return (chunk->numBlocks != 0);
}

void ChunkGenerator::TryEnqueueTree(Chunk* chunk, Biome *biome, int x, int z, int c)
{
 
    int index = FloraGenerator::getTreeIndex(biome, x, z);
    if (index == -1) return;
    chunk->treesToLoad.emplace_back();
    chunk->treesToLoad.back().startc = c;
    FloraGenerator::makeTreeData(chunk, chunk->treesToLoad.back(), GameManager::planet->treeTypeVec[index]);
}

void ChunkGenerator::LoadMinerals(Chunk* chunk)
{
    const int minh = chunk->minh;
    float chance;
    float d;
    MineralData *md;
    for (Uint32 i = 0; i < chunk->possibleMinerals.size(); i++){
        md = chunk->possibleMinerals[i];
        if (minh > md->centerHeight && minh <= md->startHeight){
            chance = ((((float)md->startHeight - minh) / ((float)md->startHeight - md->centerHeight))*(md->centerChance - md->startChance)) + md->startChance;
        } else if (minh <= md->centerHeight && minh >= md->endHeight){
            chance = ((((float)minh - md->endHeight) / ((float)md->centerHeight - md->endHeight))*(md->centerChance - md->endChance)) + md->endChance;
        } else{
            chance = 0;
        }
        d = (float)(PseudoRand(chunk->gridPosition.x + chunk->gridPosition.y - i*i * 11, chunk->gridPosition.z + 8 * i - 2 * chunk->gridPosition.y) + 1.0)*50.0;

        if (d <= chance - 10.0){ //3 ore veins
            MakeMineralVein(chunk, md, 32);
            MakeMineralVein(chunk, md, 6433);
            MakeMineralVein(chunk, md, 9189);
        } else if (d <= chance - 5.0){ //2 ore veins
            MakeMineralVein(chunk, md, 53);
            MakeMineralVein(chunk, md, 2663);
        } else if (d <= chance){
            MakeMineralVein(chunk, md, 882);
        }
    }
}

void ChunkGenerator::MakeMineralVein(Chunk* chunk, MineralData *md, int seed)
{
    int c = (int)(((PseudoRand(chunk->gridPosition.x - seed*seed + 3 * chunk->gridPosition.y + md->blockType * 2, chunk->gridPosition.z + seed * 4 - chunk->gridPosition.y + md->blockType - 44) + 1.0) / 2.0)*CHUNK_SIZE);
    int btype = md->blockType;
    int size = ((PseudoRand(chunk->gridPosition.x + 2 * chunk->gridPosition.y - md->blockType * 4 + seed, chunk->gridPosition.z + chunk->gridPosition.y - md->blockType + 44) + 1.0) / 2.0)*(md->maxSize - md->minSize) + md->minSize;
    int r;
    int x, y, z;
    for (int i = 0; i < size; i++){
        if (chunk->getBlock(c).material == M_STONE){
            chunk->setBlockData(c, btype);
        }
        x = c % CHUNK_WIDTH;
        y = c / CHUNK_LAYER;
        z = (c % CHUNK_LAYER) / CHUNK_WIDTH;

        r = (int)((PseudoRand(chunk->gridPosition.x * c + c * i + btype + seed * 2, c * c - chunk->gridPosition.z + 6 - chunk->gridPosition.y * btype - i * 3 - seed) + 1.0) * 2.5 + 0.5); //0-5
        if (r == 0 && y > 0){
            c -= CHUNK_LAYER;
        } else if (r == 1 && x > 0){
            c--;
        } else if (r == 2 && x < CHUNK_WIDTH - 1){
            c++;
        } else if (r == 3 && z > 0){
            c -= CHUNK_WIDTH;
        } else if (r == 4 && z < CHUNK_WIDTH - 1){
            c += CHUNK_WIDTH;
        } else if (y < CHUNK_WIDTH - 1){
            c += CHUNK_LAYER;
        } else{ //down by default
            c -= CHUNK_LAYER;
        }
    }
}