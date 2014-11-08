#include "stdafx.h"
#include "ChunkMesher.h"

#include <random>

#include "VoxelMesher.h"
#include "BlockData.h"
#include "Errors.h"
#include "GameManager.h"
#include "Options.h"
#include "Planet.h"
#include "RenderTask.h"
#include "TerrainGenerator.h"
#include "TexturePackLoader.h"
#include "ThreadPool.h"
#include "utils.h"
#include "VoxelUtils.h"

ChunkMesher::ChunkMesher()
{

    chunkMeshData = NULL;

}

ChunkMesher::~ChunkMesher()
{

}

void ChunkMesher::bindVBOIndicesID()
{
    vector<GLuint> indices;
    indices.resize(589824);

    int j = 0;
    for (Uint32 i = 0; i < indices.size()-12; i += 6){
        indices[i] = j;
        indices[i+1] = j+1;
        indices[i+2] = j+2;
        indices[i+3] = j+2;
        indices[i+4] = j+3;
        indices[i+5] = j;
        j += 4;
    }

    if (Chunk::vboIndicesID != 0){
        glDeleteBuffers(1, &(Chunk::vboIndicesID));
    }
    glGenBuffers(1, &(Chunk::vboIndicesID));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, (Chunk::vboIndicesID));
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 500000 * sizeof(GLuint), NULL, GL_STATIC_DRAW);
        
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, 500000 * sizeof(GLuint), &(indices[0])); //arbitrarily set to 300000
}

#define CompareVertices(v1, v2) (0 && !memcmp(&v1.color, &v2.color, 3) && v1.sunlight == v2.sunlight && !memcmp(&v1.lampColor, &v2.lampColor, 3)  \
    && !memcmp(&v1.overlayColor, &v2.overlayColor, 3) \
    && v1.textureAtlas == v2.textureAtlas && v1.textureIndex == v2.textureIndex && v1.overlayTextureAtlas == v2.overlayTextureAtlas && v1.overlayTextureIndex == v2.overlayTextureIndex)

#define CompareVerticesLight(v1, v2) (0 && v1.sunlight == v2.sunlight && !memcmp(&v1.lampColor, &v2.lampColor, 3) && !memcmp(&v1.color, &v2.color, 3))

//Fills chLightData with the lighting information of the surrounding blocks. This will be used to calculate lighting and ambient occlusion
//It only has to get the light voxels that were not grabbed in checkBlockFaces()
void ChunkMesher::GetLightDataArray(int c, int &x, int &y, int &z, ui8 lampLights[26][3], GLbyte sunlights[26], GLushort *chData, ui8* chSunData, ui16 *chLampData, bool faces[6])
{
    int c2;
    //bottom 8 voxels
    if (faces[XNEG] || faces[ZNEG] || faces[XPOS] || faces[ZPOS] || faces[YNEG]){
        c2 = c - dataLayer;
        GetBackLightData(c2, lampLights[1], sunlights[1], chData, chSunData, chLampData);
        GetLeftLightData(c2, lampLights[3], sunlights[3], chData, chSunData, chLampData);
        GetRightLightData(c2, lampLights[5], sunlights[5], chData, chSunData, chLampData);
        GetFrontLightData(c2, lampLights[7], sunlights[7], chData, chSunData, chLampData);

        GetLeftLightData(c2 - dataWidth, lampLights[0], sunlights[0], chData, chSunData, chLampData);
        GetRightLightData(c2 - dataWidth, lampLights[2], sunlights[2], chData, chSunData, chLampData);
        
        GetLeftLightData(c2 + dataWidth, lampLights[6], sunlights[6], chData, chSunData, chLampData);
        GetRightLightData(c2 + dataWidth, lampLights[8], sunlights[8], chData, chSunData, chLampData);
        
    }
    //top 8 voxels
    if (faces[XNEG] || faces[ZNEG] || faces[XPOS] || faces[ZPOS] || faces[YPOS]){
        c2 = c + dataLayer;
        GetBackLightData(c2, lampLights[18], sunlights[18], chData, chSunData, chLampData);
        GetLeftLightData(c2, lampLights[20], sunlights[20], chData, chSunData, chLampData);
        GetRightLightData(c2, lampLights[22], sunlights[22], chData, chSunData, chLampData);
        GetFrontLightData(c2, lampLights[24], sunlights[24], chData, chSunData, chLampData);
            
        GetLeftLightData(c2 - dataWidth, lampLights[17], sunlights[17], chData, chSunData, chLampData);
        GetRightLightData(c2 - dataWidth, lampLights[19], sunlights[19], chData, chSunData, chLampData);
        
        GetLeftLightData(c2 + dataWidth, lampLights[23], sunlights[23], chData, chSunData, chLampData);
        GetRightLightData(c2 + dataWidth, lampLights[25], sunlights[25], chData, chSunData, chLampData);
    }
    //left 2 voxels
    if (faces[XNEG] || faces[ZNEG] || faces[ZPOS]){
        //left
        c2 = c-1;
        GetBackLightData(c2, lampLights[9], sunlights[9], chData, chSunData, chLampData);
        GetFrontLightData(c2, lampLights[14], sunlights[14], chData, chSunData, chLampData);
    }
    //right 2 voxels
    if (faces[XPOS] || faces[ZPOS] || faces[ZNEG]){
        c2 = c+1;
        GetBackLightData(c2, lampLights[11], sunlights[11], chData, chSunData, chLampData);
        GetFrontLightData(c2, lampLights[16], sunlights[16], chData, chSunData, chLampData);
    }
}

//This function checks all surrounding blocks and stores occlusion information in faces[]. It also stores the adjacent light and sunlight for each face.
bool ChunkMesher::checkBlockFaces(bool faces[6], ui8 lampLights[26][3], i8 sunlights[26], const RenderTask* task, const BlockOcclusion occlude, const int btype, const int wc)
{
    //*** END GET_L_COLOR

    Block *nblock;
    bool hasFace = false;
    ui16 lightColor;

    if (faces[XNEG] = ((nblock = &GETBLOCK(_blockIDData[wc - 1]))->occlude == BlockOcclusion::NONE || ((nblock->occlude == BlockOcclusion::SELF || occlude == BlockOcclusion::SELF) && nblock->ID != btype))){
        hasFace = true;
        lightColor = _lampLightData[wc - 1];
        GET_L_COLOR(lampLights[12]);
        sunlights[12] = _sunlightData[wc - 1];
    }

    if (faces[XPOS] = ((nblock = &GETBLOCK(_blockIDData[1 + wc]))->occlude == BlockOcclusion::NONE || ((nblock->occlude == BlockOcclusion::SELF || occlude == BlockOcclusion::SELF) && nblock->ID != btype))){
        hasFace = true;
        lightColor = _lampLightData[1 + wc];
        GET_L_COLOR(lampLights[13]);
        sunlights[13] = (_sunlightData[1 + wc]);
    }

    if (faces[YNEG] = ((nblock = &GETBLOCK(_blockIDData[wc - dataLayer]))->occlude == BlockOcclusion::NONE || ((nblock->occlude == BlockOcclusion::SELF || occlude == BlockOcclusion::SELF) && nblock->ID != btype))){
        hasFace = true;
        lightColor = _lampLightData[wc - dataLayer];
        GET_L_COLOR(lampLights[4]);
        sunlights[4] = _sunlightData[wc - dataLayer];
    }

    if (faces[YPOS] = ((nblock = &GETBLOCK(_blockIDData[wc + dataLayer]))->occlude == BlockOcclusion::NONE || ((nblock->occlude == BlockOcclusion::SELF || occlude == BlockOcclusion::SELF) && nblock->ID != btype))){
        hasFace = true;
        lightColor = _lampLightData[wc + dataLayer];
        GET_L_COLOR(lampLights[21]);
        sunlights[21] = _sunlightData[wc + dataLayer];
    }

    if (faces[ZNEG] = ((nblock = &GETBLOCK(_blockIDData[wc - dataWidth]))->occlude == BlockOcclusion::NONE || ((nblock->occlude == BlockOcclusion::SELF || occlude == BlockOcclusion::SELF) && nblock->ID != btype))){
        hasFace = true;
        lightColor = _lampLightData[wc - dataWidth];
        GET_L_COLOR(lampLights[10]);
        sunlights[10] = _sunlightData[wc - dataWidth];
    }

    if (faces[ZPOS] = ((nblock = &GETBLOCK(_blockIDData[wc + dataWidth]))->occlude == BlockOcclusion::NONE || ((nblock->occlude == BlockOcclusion::SELF || occlude == BlockOcclusion::SELF) && nblock->ID != btype))){
        hasFace = true;
        lightColor = _lampLightData[wc + dataWidth];
        GET_L_COLOR(lampLights[15]);
        sunlights[15] = _sunlightData[wc + dataWidth];
    }

    return hasFace;
}

//Calculates the smooth lighting based on accumulated light and num adjacent blocks.
//Returns it as a GLubyte for vertex data
GLubyte ChunkMesher::calculateSmoothLighting(int accumulatedLight, int numAdjacentBlocks) {
    return (GLubyte)(255.0f * (LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - ((float)(accumulatedLight) / (4 - numAdjacentBlocks)))));
}

void ChunkMesher::calculateLampColor(ColorRGB8& dst, ui8 src0[3], ui8 src1[3], ui8 src2[3], ui8 src3[3], ui8 numAdj) {
    dst.r = calculateSmoothLighting(src0[0] + src1[0] + src2[0] + src3[0], numAdj);
    dst.g = calculateSmoothLighting(src0[1] + src1[1] + src2[1] + src3[1], numAdj);
    dst.b = calculateSmoothLighting(src0[2] + src1[2] + src2[2] + src3[2], numAdj);
}

void ChunkMesher::addBlockToMesh(MesherInfo& mi)
{
    const Block &block = Blocks[mi.btype];

    ui8 lampLights[26][3];
    GLbyte sunlights[26];
    ColorRGB8 color, overlayColor;
    const int wc = mi.wc;
    const int btype = mi.btype;
    int nearBlocks[4];
    int textureIndex, overlayTextureIndex;
    const GLfloat OCCLUSION_FACTOR = 0.2f;

    GLfloat ambientOcclusion[4] = { 1.0f, 1.0f, 1.0f, 1.0f };

    ui8 sunLight = _sunlightData[wc];
    ColorRGB8 lampLight((_lampLightData[wc] & LAMP_RED_MASK) >> LAMP_RED_SHIFT,
        (_lampLightData[wc] & LAMP_GREEN_MASK) >> LAMP_GREEN_SHIFT,
        _lampLightData[wc] & LAMP_BLUE_MASK);

    sunLight = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - sunLight)));
    lampLight.r = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.r)));
    lampLight.g = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.g)));
    lampLight.b = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.b)));

    //Lookup the current biome, temperature, and rainfall
    Biome *biome = mi.chunkGridData->heightData[mi.nz*CHUNK_WIDTH + mi.nx].biome;
    mi.temperature = mi.chunkGridData->heightData[mi.nz*CHUNK_WIDTH + mi.nx].temperature;
    mi.rainfall = mi.chunkGridData->heightData[mi.nz*CHUNK_WIDTH + mi.nx].rainfall;

    //get bit flags (needs to be changed) -Ben
    GLuint flags = GETFLAGS(mi.task->chData[mi.wc]);

    bool faces[6] = { false, false, false, false, false, false };
    //Check for which faces are occluded by nearby blocks
    if (checkBlockFaces(faces, lampLights, sunlights, mi.task, block.occlude, btype, wc) == 0) {
        mi.mergeFront = 0;
        mi.mergeBack = 0;
        mi.mergeBot = 0;
        mi.mergeUp = 0;
        return;
    }

    //Get nearby lighting information

    GetLightDataArray(wc, mi.x, mi.y, mi.z, lampLights, sunlights, mi.task->chData, mi.task->chSunlightData, mi.task->chLampData, faces);

    if (faces[ZPOS]){ //0 1 2 3
        //Get the color of the block
        Blocks[btype].GetBlockColor(color, overlayColor, flags, mi.temperature, mi.rainfall, block.pzTexInfo);

        //Count the number of solid blocks ad add them to near blocks for ambient occlusion
        nearBlocks[0] = (int)(sunlights[23] == -1) + (int)(sunlights[24] == -1) + (int)(sunlights[14] == -1);
        nearBlocks[1] = (int)(sunlights[6] == -1) + (int)(sunlights[7] == -1) + (int)(sunlights[14] == -1);
        nearBlocks[2] = (int)(sunlights[7] == -1) + (int)(sunlights[8] == -1) + (int)(sunlights[16] == -1);
        nearBlocks[3] = (int)(sunlights[24] == -1) + (int)(sunlights[25] == -1) + (int)(sunlights[16] == -1);
        //Calculate ambient occlusion based on nearby blocks

        if (mi.levelOfDetail == 1) {
            ambientOcclusion[0] = 1.0f - nearBlocks[0] * OCCLUSION_FACTOR;
            ambientOcclusion[1] = 1.0f - nearBlocks[1] * OCCLUSION_FACTOR;
            ambientOcclusion[2] = 1.0f - nearBlocks[2] * OCCLUSION_FACTOR;
            ambientOcclusion[3] = 1.0f - nearBlocks[3] * OCCLUSION_FACTOR;
        }

        //We will offset the texture index based on the texture method
        textureIndex = block.pzTexInfo.base.getBlockTextureIndex(mi.pzBaseMethodParams, color);
        overlayTextureIndex = block.pzTexInfo.overlay.getBlockTextureIndex(mi.pzOverlayMethodParams, overlayColor);

        if (block.pzTexInfo.base.transparency) {
            _transparentVerts.resize(_transparentVerts.size() + 4);
            VoxelMesher::makeFloraFace(&(_transparentVerts[0]), VoxelMesher::cubeVertices[mi.levelOfDetail - 1], VoxelMesher::cubeNormals, 0, block.waveEffect, i32v3(mi.x, mi.y, mi.z), mi.transparentIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pzTexInfo);
            mi.transparentIndex += 4;

            chunkMeshData->addTransQuad(i8v3(mi.x2 + 1, mi.y2 + 1, mi.z2 + 2));

            mi.mergeFront = false;
        } else {
            //Set up most of the vertex data for a face
            VoxelMesher::makeCubeFace(_frontVerts, mi.levelOfDetail, CUBE_FACE_0_VERTEX_OFFSET, (int)block.waveEffect, glm::ivec3(mi.nx, mi.ny, mi.nz), mi.frontIndex, textureIndex, overlayTextureIndex, color, overlayColor, ambientOcclusion, block.pzTexInfo);

            //Set up the light data using smooth lighting

            calculateLampColor(_frontVerts[mi.frontIndex].lampColor, lampLights[23], lampLights[24], lampLights[14], lampLights[15], nearBlocks[0]);
            calculateLampColor(_frontVerts[mi.frontIndex + 1].lampColor, lampLights[6], lampLights[7], lampLights[14], lampLights[15], nearBlocks[1]);
            calculateLampColor(_frontVerts[mi.frontIndex + 2].lampColor, lampLights[7], lampLights[8], lampLights[15], lampLights[16], nearBlocks[2]);
            calculateLampColor(_frontVerts[mi.frontIndex + 3].lampColor, lampLights[24], lampLights[25], lampLights[15], lampLights[16], nearBlocks[3]);
            _frontVerts[mi.frontIndex].sunlight = calculateSmoothLighting(sunlights[23] + sunlights[24] + sunlights[14] + sunlights[15], nearBlocks[0]);
            _frontVerts[mi.frontIndex + 1].sunlight = calculateSmoothLighting(sunlights[6] + sunlights[7] + sunlights[14] + sunlights[15], nearBlocks[1]);
            _frontVerts[mi.frontIndex + 2].sunlight = calculateSmoothLighting(sunlights[7] + sunlights[8] + sunlights[15] + sunlights[16], nearBlocks[2]);
            _frontVerts[mi.frontIndex + 3].sunlight = calculateSmoothLighting(sunlights[24] + sunlights[25] + sunlights[15] + sunlights[16], nearBlocks[3]);

            //to check for a +x merge, we check that the vertices aligned in the direction of stretch are equal
            if (mi.mergeFront && mi.pbtype == btype &&
                CompareVertices(_frontVerts[mi.pfrontIndex], _frontVerts[mi.pfrontIndex + 3]) && CompareVertices(_frontVerts[mi.pfrontIndex + 3], _frontVerts[mi.frontIndex]) && CompareVertices(_frontVerts[mi.frontIndex], _frontVerts[mi.frontIndex + 3]) &&//-z vertices
                CompareVertices(_frontVerts[mi.pfrontIndex + 1], _frontVerts[mi.pfrontIndex + 2]) && CompareVertices(_frontVerts[mi.pfrontIndex + 2], _frontVerts[mi.frontIndex + 1]) && CompareVertices(_frontVerts[mi.frontIndex + 1], _frontVerts[mi.frontIndex + 2])){ //+z vertices

                _frontVerts[mi.pfrontIndex + 2].position.x += 7;
                _frontVerts[mi.pfrontIndex + 3].position.x += 7;
                _frontVerts[mi.pfrontIndex + 2].tex[0]++;
                _frontVerts[mi.pfrontIndex + 3].tex[0]++;
            } else{
                mi.pfrontIndex = mi.frontIndex;
                mi.frontIndex += 4;
                mi.mergeFront = 1;
            }
        }
    }
    else{
        mi.mergeFront = 0;
    }

    if (faces[ZNEG]) {
        //Get the color of the block
        Blocks[btype].GetBlockColor(color, overlayColor, flags, mi.temperature, mi.rainfall, block.nzTexInfo);

        nearBlocks[0] = (int)(sunlights[18] == -1) + (int)(sunlights[19] == -1) + (int)(sunlights[11] == -1);
        nearBlocks[1] = (int)(sunlights[1] == -1) + (int)(sunlights[2] == -1) + (int)(sunlights[11] == -1);
        nearBlocks[2] = (int)(sunlights[0] == -1) + (int)(sunlights[1] == -1) + (int)(sunlights[9] == -1);
        nearBlocks[3] = (int)(sunlights[17] == -1) + (int)(sunlights[18] == -1) + (int)(sunlights[9] == -1);
        if (mi.levelOfDetail == 1) {
            ambientOcclusion[0] = 1.0f - nearBlocks[0] * OCCLUSION_FACTOR;
            ambientOcclusion[1] = 1.0f - nearBlocks[1] * OCCLUSION_FACTOR;
            ambientOcclusion[2] = 1.0f - nearBlocks[2] * OCCLUSION_FACTOR;
            ambientOcclusion[3] = 1.0f - nearBlocks[3] * OCCLUSION_FACTOR;
        }
       
        //We will offset the texture index based on the texture method

        textureIndex = block.nzTexInfo.base.getBlockTextureIndex(mi.nzBaseMethodParams, color);
        overlayTextureIndex = block.nzTexInfo.overlay.getBlockTextureIndex(mi.nzOverlayMethodParams, overlayColor);

        if (block.nzTexInfo.base.transparency) {
            _transparentVerts.resize(_transparentVerts.size() + 4);
            VoxelMesher::makeFloraFace(&(_transparentVerts[0]), VoxelMesher::cubeVertices[mi.levelOfDetail - 1], VoxelMesher::cubeNormals, 60, block.waveEffect, i32v3(mi.x, mi.y, mi.z), mi.transparentIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.nzTexInfo);
            mi.transparentIndex += 4;

            chunkMeshData->addTransQuad(i8v3(mi.x2 + 1, mi.y2 + 1, mi.z2));
            mi.mergeBack = false;
        } else {
            VoxelMesher::makeCubeFace(_backVerts, mi.levelOfDetail, CUBE_FACE_5_VERTEX_OFFSET, (int)block.waveEffect, glm::ivec3(mi.nx, mi.ny, mi.nz), mi.backIndex, textureIndex, overlayTextureIndex, color, overlayColor, ambientOcclusion, block.nzTexInfo);

            calculateLampColor(_backVerts[mi.backIndex].lampColor, lampLights[18], lampLights[19], lampLights[11], lampLights[10], nearBlocks[0]);
            calculateLampColor(_backVerts[mi.backIndex + 1].lampColor, lampLights[1], lampLights[2], lampLights[11], lampLights[10], nearBlocks[1]);
            calculateLampColor(_backVerts[mi.backIndex + 2].lampColor, lampLights[0], lampLights[1], lampLights[9], lampLights[10], nearBlocks[2]);
            calculateLampColor(_backVerts[mi.backIndex + 3].lampColor, lampLights[17], lampLights[18], lampLights[9], lampLights[10], nearBlocks[3]);
            _backVerts[mi.backIndex].sunlight = calculateSmoothLighting(sunlights[18] + sunlights[19] + sunlights[11] + sunlights[10], nearBlocks[0]);
            _backVerts[mi.backIndex + 1].sunlight = calculateSmoothLighting(sunlights[1] + sunlights[2] + sunlights[11] + sunlights[10], nearBlocks[1]);
            _backVerts[mi.backIndex + 2].sunlight = calculateSmoothLighting(sunlights[0] + sunlights[1] + sunlights[9] + sunlights[10], nearBlocks[2]);
            _backVerts[mi.backIndex + 3].sunlight = calculateSmoothLighting(sunlights[17] + sunlights[18] + sunlights[9] + sunlights[10], nearBlocks[3]);

            //to check for a +x merge, we check that the vertices aligned in the direction of stretch are equal
            if (mi.mergeBack && mi.pbtype == btype &&
                CompareVertices(_backVerts[mi.pbackIndex], _backVerts[mi.pbackIndex + 3]) && CompareVertices(_backVerts[mi.pbackIndex + 3], _backVerts[mi.backIndex]) && CompareVertices(_backVerts[mi.backIndex], _backVerts[mi.backIndex + 3]) &&//-z vertices
                CompareVertices(_backVerts[mi.pbackIndex + 1], _backVerts[mi.pbackIndex + 2]) && CompareVertices(_backVerts[mi.pbackIndex + 2], _backVerts[mi.backIndex + 1]) && CompareVertices(_backVerts[mi.backIndex + 1], _backVerts[mi.backIndex + 2])){ //+z vertices

                _backVerts[mi.pbackIndex].position.x += 7;
                _backVerts[mi.pbackIndex + 1].position.x += 7;
                _backVerts[mi.pbackIndex].tex[0]--;
                _backVerts[mi.pbackIndex + 1].tex[0]--;
            } else{
                mi.pbackIndex = mi.backIndex;
                mi.backIndex += 4;
                mi.mergeBack = 1;
            }
        }
    } else {
        mi.mergeBack = 0;
    }

    if (faces[YPOS]){ //0 5 6 1                        
        Blocks[btype].GetBlockColor(color, overlayColor, flags, mi.temperature, mi.rainfall, block.pyTexInfo);

        nearBlocks[0] = (int)(sunlights[17] == -1) + (int)(sunlights[18] == -1) + (int)(sunlights[20] == -1);
        nearBlocks[1] = (int)(sunlights[20] == -1) + (int)(sunlights[23] == -1) + (int)(sunlights[24] == -1);
        nearBlocks[2] = (int)(sunlights[22] == -1) + (int)(sunlights[24] == -1) + (int)(sunlights[25] == -1);
        nearBlocks[3] = (int)(sunlights[18] == -1) + (int)(sunlights[19] == -1) + (int)(sunlights[22] == -1);
        if (mi.levelOfDetail == 1) {
            ambientOcclusion[0] = 1.0f - nearBlocks[0] * OCCLUSION_FACTOR;
            ambientOcclusion[1] = 1.0f - nearBlocks[1] * OCCLUSION_FACTOR;
            ambientOcclusion[2] = 1.0f - nearBlocks[2] * OCCLUSION_FACTOR;
            ambientOcclusion[3] = 1.0f - nearBlocks[3] * OCCLUSION_FACTOR;
        }

        //We will offset the texture index based on the texture method
        textureIndex = block.pyTexInfo.base.getBlockTextureIndex(mi.pyBaseMethodParams, color);
        overlayTextureIndex = block.pyTexInfo.overlay.getBlockTextureIndex(mi.pyOverlayMethodParams, overlayColor);

        if (block.pyTexInfo.base.transparency) {
            _transparentVerts.resize(_transparentVerts.size() + 4);
            VoxelMesher::makeFloraFace(&(_transparentVerts[0]), VoxelMesher::cubeVertices[mi.levelOfDetail - 1], VoxelMesher::cubeNormals, 24, block.waveEffect, i32v3(mi.x, mi.y, mi.z), mi.transparentIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pyTexInfo);
            mi.transparentIndex += 4;

            chunkMeshData->addTransQuad(i8v3(mi.x2 + 1, mi.y2 + 2, mi.z2 + 1));

            mi.mergeUp = false;
        } else {
            VoxelMesher::makeCubeFace(_topVerts, mi.levelOfDetail, CUBE_FACE_2_VERTEX_OFFSET, (int)block.waveEffect, glm::ivec3(mi.nx, mi.ny, mi.nz), mi.topIndex, textureIndex, overlayTextureIndex, color, overlayColor, ambientOcclusion, block.pyTexInfo);

            //Check for +x direction merging
            calculateLampColor(_topVerts[mi.topIndex].lampColor, lampLights[17], lampLights[18], lampLights[20], lampLights[21], nearBlocks[0]);
            calculateLampColor(_topVerts[mi.topIndex + 1].lampColor, lampLights[20], lampLights[21], lampLights[23], lampLights[24], nearBlocks[1]);
            calculateLampColor(_topVerts[mi.topIndex + 2].lampColor, lampLights[21], lampLights[22], lampLights[24], lampLights[25], nearBlocks[2]);
            calculateLampColor(_topVerts[mi.topIndex + 3].lampColor, lampLights[18], lampLights[19], lampLights[21], lampLights[22], nearBlocks[3]);
            _topVerts[mi.topIndex].sunlight = calculateSmoothLighting(sunlights[17] + sunlights[18] + sunlights[20] + sunlights[21], nearBlocks[0]);
            _topVerts[mi.topIndex + 1].sunlight = calculateSmoothLighting(sunlights[20] + sunlights[21] + sunlights[23] + sunlights[24], nearBlocks[1]);
            _topVerts[mi.topIndex + 2].sunlight = calculateSmoothLighting(sunlights[21] + sunlights[22] + sunlights[24] + sunlights[25], nearBlocks[2]);
            _topVerts[mi.topIndex + 3].sunlight = calculateSmoothLighting(sunlights[18] + sunlights[19] + sunlights[21] + sunlights[22], nearBlocks[3]);

            //to check for a +x merge, we check that the vertices aligned in the direction of stretch are equal
            if (mi.mergeUp && mi.pbtype == btype &&
                CompareVertices(_topVerts[mi.pupIndex], _topVerts[mi.pupIndex + 3]) && CompareVertices(_topVerts[mi.pupIndex + 3], _topVerts[mi.topIndex]) && CompareVertices(_topVerts[mi.topIndex], _topVerts[mi.topIndex + 3]) &&//-z vertices
                CompareVertices(_topVerts[mi.pupIndex + 1], _topVerts[mi.pupIndex + 2]) && CompareVertices(_topVerts[mi.pupIndex + 2], _topVerts[mi.topIndex + 1]) && CompareVertices(_topVerts[mi.topIndex + 1], _topVerts[mi.topIndex + 2])){ //+z vertices
                _topVerts[mi.pupIndex + 2].position.x += 7;                    //change x
                _topVerts[mi.pupIndex + 3].position.x += 7;
                _topVerts[mi.pupIndex + 2].tex[0]++;
                _topVerts[mi.pupIndex + 3].tex[0]++;
            } else{
                mi.pupIndex = mi.topIndex;
                mi.topIndex += 4;
                mi.mergeUp = true;
            }
        }
    }
    else{
        mi.mergeUp = false;
    }

    if (faces[YNEG]) {
        Blocks[btype].GetBlockColor(color, overlayColor, flags, mi.temperature, mi.rainfall, block.nyTexInfo);

        nearBlocks[0] = (int)(sunlights[1] == -1) + (int)(sunlights[2] == -1) + (int)(sunlights[5] == -1);
        nearBlocks[1] = (int)(sunlights[5] == -1) + (int)(sunlights[7] == -1) + (int)(sunlights[8] == -1);
        nearBlocks[2] = (int)(sunlights[3] == -1) + (int)(sunlights[6] == -1) + (int)(sunlights[7] == -1);
        nearBlocks[3] = (int)(sunlights[0] == -1) + (int)(sunlights[1] == -1) + (int)(sunlights[3] == -1);
        if (mi.levelOfDetail == 1) {
            ambientOcclusion[0] = 1.0f - nearBlocks[0] * OCCLUSION_FACTOR;
            ambientOcclusion[1] = 1.0f - nearBlocks[1] * OCCLUSION_FACTOR;
            ambientOcclusion[2] = 1.0f - nearBlocks[2] * OCCLUSION_FACTOR;
            ambientOcclusion[3] = 1.0f - nearBlocks[3] * OCCLUSION_FACTOR;
        }
       
        //We will offset the texture index based on the texture method
        textureIndex = block.nyTexInfo.base.getBlockTextureIndex(mi.nyBaseMethodParams, color);
        overlayTextureIndex = block.nyTexInfo.overlay.getBlockTextureIndex(mi.nyOverlayMethodParams, overlayColor);

        if (block.nyTexInfo.base.transparency) {
            _transparentVerts.resize(_transparentVerts.size() + 4);
            VoxelMesher::makeFloraFace(&(_transparentVerts[0]), VoxelMesher::cubeVertices[mi.levelOfDetail - 1], VoxelMesher::cubeNormals, 48, block.waveEffect, i32v3(mi.x, mi.y, mi.z), mi.transparentIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.nyTexInfo);
            mi.transparentIndex += 4;

            chunkMeshData->addTransQuad(i8v3(mi.x2 + 1, mi.y2, mi.z2 + 1));

            mi.mergeBot = false;
        } else {

            VoxelMesher::makeCubeFace(_bottomVerts, mi.levelOfDetail, CUBE_FACE_4_VERTEX_OFFSET, (int)block.waveEffect, glm::ivec3(mi.nx, mi.ny, mi.nz), mi.botIndex, textureIndex, overlayTextureIndex, color, overlayColor, ambientOcclusion, block.nyTexInfo);

            calculateLampColor(_bottomVerts[mi.botIndex].lampColor, lampLights[1], lampLights[2], lampLights[4], lampLights[5], nearBlocks[0]);
            calculateLampColor(_bottomVerts[mi.botIndex + 1].lampColor, lampLights[4], lampLights[5], lampLights[7], lampLights[8], nearBlocks[1]);
            calculateLampColor(_bottomVerts[mi.botIndex + 2].lampColor, lampLights[3], lampLights[4], lampLights[6], lampLights[7], nearBlocks[2]);
            calculateLampColor(_bottomVerts[mi.botIndex + 3].lampColor, lampLights[0], lampLights[1], lampLights[3], lampLights[4], nearBlocks[3]);
            _bottomVerts[mi.botIndex].sunlight = calculateSmoothLighting(sunlights[1] + sunlights[2] + sunlights[4] + sunlights[5], nearBlocks[0]);
            _bottomVerts[mi.botIndex + 1].sunlight = calculateSmoothLighting(sunlights[4] + sunlights[5] + sunlights[7] + sunlights[8], nearBlocks[1]);
            _bottomVerts[mi.botIndex + 2].sunlight = calculateSmoothLighting(sunlights[3] + sunlights[4] + sunlights[6] + sunlights[7], nearBlocks[2]);
            _bottomVerts[mi.botIndex + 3].sunlight = calculateSmoothLighting(sunlights[0] + sunlights[1] + sunlights[3] + sunlights[4], nearBlocks[3]);

            //to check for a +x merge, we check that the vertices aligned in the direction of stretch are equal
            if (mi.mergeBot && mi.pbtype == btype &&
                CompareVertices(_bottomVerts[mi.pbotIndex], _bottomVerts[mi.pbotIndex + 3]) && CompareVertices(_bottomVerts[mi.pbotIndex + 3], _bottomVerts[mi.botIndex]) && CompareVertices(_bottomVerts[mi.botIndex], _bottomVerts[mi.botIndex + 3]) &&//-z vertices
                CompareVertices(_bottomVerts[mi.pbotIndex + 1], _bottomVerts[mi.pbotIndex + 2]) && CompareVertices(_bottomVerts[mi.pbotIndex + 2], _bottomVerts[mi.botIndex + 1]) && CompareVertices(_bottomVerts[mi.botIndex + 1], _bottomVerts[mi.botIndex + 2])){ //+z vertices
                _bottomVerts[mi.pbotIndex].position.x += 7;                    //change x
                _bottomVerts[mi.pbotIndex + 1].position.x += 7;
                _bottomVerts[mi.pbotIndex].tex[0]--;
                _bottomVerts[mi.pbotIndex + 1].tex[0]--;
            } else {
                mi.pbotIndex = mi.botIndex;
                mi.botIndex += 4;
                mi.mergeBot = 1;
            }
        }
    } else{
        mi.mergeBot = 0;
    }

    if (faces[XPOS]) {
        Blocks[btype].GetBlockColor(color, overlayColor, flags, mi.temperature, mi.rainfall, block.pxTexInfo);

        nearBlocks[0] = (int)(sunlights[25] == -1) + (int)(sunlights[22] == -1) + (int)(sunlights[16] == -1);
        nearBlocks[1] = (int)(sunlights[5] == -1) + (int)(sunlights[8] == -1) + (int)(sunlights[16] == -1);
        nearBlocks[2] = (int)(sunlights[2] == -1) + (int)(sunlights[5] == -1) + (int)(sunlights[11] == -1);
        nearBlocks[3] = (int)(sunlights[19] == -1) + (int)(sunlights[22] == -1) + (int)(sunlights[11] == -1);
        if (mi.levelOfDetail == 1) {
            ambientOcclusion[0] = 1.0f - nearBlocks[0] * OCCLUSION_FACTOR;
            ambientOcclusion[1] = 1.0f - nearBlocks[1] * OCCLUSION_FACTOR;
            ambientOcclusion[2] = 1.0f - nearBlocks[2] * OCCLUSION_FACTOR;
            ambientOcclusion[3] = 1.0f - nearBlocks[3] * OCCLUSION_FACTOR;
        }
      
        //We will offset the texture index based on the texture method
        textureIndex = block.pxTexInfo.base.getBlockTextureIndex(mi.pxBaseMethodParams, color);
        overlayTextureIndex = block.pxTexInfo.overlay.getBlockTextureIndex(mi.pxOverlayMethodParams, overlayColor);


        if (block.pxTexInfo.base.transparency) {  
            _transparentVerts.resize(_transparentVerts.size() + 4);
            VoxelMesher::makeFloraFace(&(_transparentVerts[0]), VoxelMesher::cubeVertices[mi.levelOfDetail - 1], VoxelMesher::cubeNormals, 12, block.waveEffect, i32v3(mi.x, mi.y, mi.z), mi.transparentIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pxTexInfo);
            mi.transparentIndex += 4;

            chunkMeshData->addTransQuad(i8v3(mi.x2 + 2, mi.y2 + 1, mi.z2 + 1));
        } else {
            VoxelMesher::makeCubeFace(_rightVerts, mi.levelOfDetail, CUBE_FACE_1_VERTEX_OFFSET, (int)block.waveEffect, glm::ivec3(mi.nx, mi.ny, mi.nz), mi.rightIndex, textureIndex, overlayTextureIndex, color, overlayColor, ambientOcclusion, block.pxTexInfo);

            calculateLampColor(_rightVerts[mi.rightIndex].lampColor, lampLights[25], lampLights[22], lampLights[13], lampLights[16], nearBlocks[0]);
            calculateLampColor(_rightVerts[mi.rightIndex + 1].lampColor, lampLights[5], lampLights[8], lampLights[13], lampLights[16], nearBlocks[1]);
            calculateLampColor(_rightVerts[mi.rightIndex + 2].lampColor, lampLights[2], lampLights[5], lampLights[11], lampLights[13], nearBlocks[2]);
            calculateLampColor(_rightVerts[mi.rightIndex + 3].lampColor, lampLights[19], lampLights[22], lampLights[11], lampLights[13], nearBlocks[3]);
            _rightVerts[mi.rightIndex].sunlight = calculateSmoothLighting(sunlights[25] + sunlights[22] + sunlights[13] + sunlights[16], nearBlocks[0]);
            _rightVerts[mi.rightIndex + 1].sunlight = calculateSmoothLighting(sunlights[5] + sunlights[8] + sunlights[13] + sunlights[16], nearBlocks[1]);
            _rightVerts[mi.rightIndex + 2].sunlight = calculateSmoothLighting(sunlights[2] + sunlights[5] + sunlights[11] + sunlights[13], nearBlocks[2]);
            _rightVerts[mi.rightIndex + 3].sunlight = calculateSmoothLighting(sunlights[19] + sunlights[22] + sunlights[11] + sunlights[13], nearBlocks[3]);

            mi.rightIndex += 4;
        }
    }

    if (faces[XNEG]) {
        Blocks[btype].GetBlockColor(color, overlayColor, flags, mi.temperature, mi.rainfall, block.nxTexInfo);

        nearBlocks[0] = (int)(sunlights[17] == -1) + (int)(sunlights[20] == -1) + (int)(sunlights[9] == -1);
        nearBlocks[1] = (int)(sunlights[0] == -1) + (int)(sunlights[3] == -1) + (int)(sunlights[9] == -1);
        nearBlocks[2] = (int)(sunlights[3] == -1) + (int)(sunlights[6] == -1) + (int)(sunlights[14] == -1);
        nearBlocks[3] = (int)(sunlights[20] == -1) + (int)(sunlights[23] == -1) + (int)(sunlights[14] == -1);
        if (mi.levelOfDetail == 1) {
            ambientOcclusion[0] = 1.0f - nearBlocks[0] * OCCLUSION_FACTOR;
            ambientOcclusion[1] = 1.0f - nearBlocks[1] * OCCLUSION_FACTOR;
            ambientOcclusion[2] = 1.0f - nearBlocks[2] * OCCLUSION_FACTOR;
            ambientOcclusion[3] = 1.0f - nearBlocks[3] * OCCLUSION_FACTOR;
        }

        //We will offset the texture index based on the texture method
        textureIndex = block.nxTexInfo.base.getBlockTextureIndex(mi.nxBaseMethodParams, color);
        overlayTextureIndex = block.nxTexInfo.overlay.getBlockTextureIndex(mi.nxOverlayMethodParams, overlayColor);

        if (block.nxTexInfo.base.transparency) {
            _transparentVerts.resize(_transparentVerts.size() + 4);
            VoxelMesher::makeFloraFace(&(_transparentVerts[0]), VoxelMesher::cubeVertices[mi.levelOfDetail - 1], VoxelMesher::cubeNormals, 36, block.waveEffect, i32v3(mi.x, mi.y, mi.z), mi.transparentIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.nxTexInfo);
            mi.transparentIndex += 4;

            chunkMeshData->addTransQuad(i8v3(mi.x2, mi.y2 + 1, mi.z2 + 1));
        } else {
            VoxelMesher::makeCubeFace(_leftVerts, mi.levelOfDetail, CUBE_FACE_3_VERTEX_OFFSET, (int)block.waveEffect, glm::ivec3(mi.nx, mi.ny, mi.nz), mi.leftIndex, textureIndex, overlayTextureIndex, color, overlayColor, ambientOcclusion, block.nxTexInfo);

            calculateLampColor(_leftVerts[mi.leftIndex].lampColor, lampLights[17], lampLights[20], lampLights[9], lampLights[12], nearBlocks[0]);
            calculateLampColor(_leftVerts[mi.leftIndex + 1].lampColor, lampLights[0], lampLights[3], lampLights[9], lampLights[12], nearBlocks[1]);
            calculateLampColor(_leftVerts[mi.leftIndex + 2].lampColor, lampLights[3], lampLights[6], lampLights[14], lampLights[12], nearBlocks[2]);
            calculateLampColor(_leftVerts[mi.leftIndex + 3].lampColor, lampLights[20], lampLights[23], lampLights[14], lampLights[12], nearBlocks[3]);
            _leftVerts[mi.leftIndex].sunlight = calculateSmoothLighting(sunlights[17] + sunlights[20] + sunlights[9] + sunlights[12], nearBlocks[0]);
            _leftVerts[mi.leftIndex + 1].sunlight = calculateSmoothLighting(sunlights[0] + sunlights[3] + sunlights[9] + sunlights[12], nearBlocks[1]);
            _leftVerts[mi.leftIndex + 2].sunlight = calculateSmoothLighting(sunlights[3] + sunlights[6] + sunlights[14] + sunlights[12], nearBlocks[2]);
            _leftVerts[mi.leftIndex + 3].sunlight = calculateSmoothLighting(sunlights[20] + sunlights[23] + sunlights[14] + sunlights[12], nearBlocks[3]);

            mi.leftIndex += 4;
        }
    }

}

//adds a flora mesh
void ChunkMesher::addFloraToMesh(MesherInfo& mi) {

    const Block &block = Blocks[mi.btype];
    mi.currentBlock = &block;

    ColorRGB8 color, overlayColor;
    const int wc = mi.wc;
    const int btype = mi.btype;

    Biome *biome = mi.chunkGridData->heightData[mi.nz*CHUNK_WIDTH + mi.nx].biome;
    int temperature = mi.chunkGridData->heightData[mi.nz*CHUNK_WIDTH + mi.nx].temperature;
    int rainfall = mi.chunkGridData->heightData[mi.nz*CHUNK_WIDTH + mi.nx].rainfall;

    GLuint flags = GETFLAGS(_blockIDData[mi.wc]);

    ui8 sunLight = mi.task->chSunlightData[wc];
    ColorRGB8 lampLight((mi.task->chLampData[wc] & LAMP_RED_MASK) >> LAMP_RED_SHIFT,
        (mi.task->chLampData[wc] & LAMP_GREEN_MASK) >> LAMP_GREEN_SHIFT,
        mi.task->chLampData[wc] & LAMP_BLUE_MASK);

    sunLight = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - sunLight)));
    lampLight.r = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.r)));
    lampLight.g = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.g)));
    lampLight.b = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.b)));

    Blocks[btype].GetBlockColor(color, overlayColor, flags, temperature, rainfall, block.pxTexInfo);

    //We will offset the texture index based on the texture method
    i32 textureIndex = block.pxTexInfo.base.getBlockTextureIndex(mi.pxBaseMethodParams, color);
    i32 overlayTextureIndex = block.pxTexInfo.overlay.getBlockTextureIndex(mi.pxOverlayMethodParams, overlayColor);

    int r;

    switch (mi.meshType){
        case MeshType::LEAVES:

            break;
        case MeshType::CROSSFLORA:
            //Generate a random number between 0 and 3 inclusive
            r = std::bind(std::uniform_int_distribution<int>(0, 1), mt19937(getPositionSeed(mi.nx, mi.nz)))();

            _cutoutVerts.resize(_cutoutVerts.size() + 8);
            VoxelMesher::makeFloraFace(&(_cutoutVerts[0]), VoxelMesher::crossFloraVertices[r], VoxelMesher::floraNormals, 0, block.waveEffect, i32v3(mi.nx, mi.ny, mi.nz), mi.cutoutIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pxTexInfo);
            mi.cutoutIndex += 4;
            VoxelMesher::makeFloraFace(&(_cutoutVerts[0]), VoxelMesher::crossFloraVertices[r], VoxelMesher::floraNormals, 12, block.waveEffect, i32v3(mi.nx, mi.ny, mi.nz), mi.cutoutIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pxTexInfo);
            mi.cutoutIndex += 4;
            break;
        case MeshType::FLORA:
            //Generate a random number between 0 and 3 inclusive
            r = std::bind(std::uniform_int_distribution<int>(0, 3), mt19937(getPositionSeed(mi.nx, mi.nz)))();

            _cutoutVerts.resize(_cutoutVerts.size() + 12);
            VoxelMesher::makeFloraFace(&(_cutoutVerts[0]), VoxelMesher::floraVertices[r], VoxelMesher::floraNormals, 0, block.waveEffect, i32v3(mi.nx, mi.ny, mi.nz), mi.cutoutIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pxTexInfo);
            mi.cutoutIndex += 4;
            VoxelMesher::makeFloraFace(&(_cutoutVerts[0]), VoxelMesher::floraVertices[r], VoxelMesher::floraNormals, 12, block.waveEffect, i32v3(mi.nx, mi.ny, mi.nz), mi.cutoutIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pxTexInfo);
            mi.cutoutIndex += 4;
            VoxelMesher::makeFloraFace(&(_cutoutVerts[0]), VoxelMesher::floraVertices[r], VoxelMesher::floraNormals, 24, block.waveEffect, i32v3(mi.nx, mi.ny, mi.nz), mi.cutoutIndex, textureIndex, overlayTextureIndex, color, overlayColor, sunLight, lampLight, block.pxTexInfo);
            mi.cutoutIndex += 4;
            break;
    }
}

//Gets the liquid level from a block index
#define LEVEL(i) ((task->chData[i] == 0) ? 0 : (((nextBlock = &GETBLOCK(task->chData[i]))->physicsProperty == block.physicsProperty) ? nextBlock->waterMeshLevel : 0))

#define CALCULATE_LIQUID_VERTEX_HEIGHT(height, heightA, heightB, cornerIndex) \
    div = 0; \
    tot = 0; \
    if (heightA) { \
        tot += heightA; \
        div++; \
    } \
    \
    if (heightB) { \
        tot += heightB; \
        div++; \
    } \
    \
    if (div) { \
        int lvl = LEVEL(cornerIndex); \
        if (lvl) { \
            tot += lvl; \
            div++; \
        } \
        height = (height + (tot / (float)maxLevel)) / (div + 1); \
    } 
    
   
//END CALCULATE_LIQUID_VERTEX_HEIGHT

void ChunkMesher::addLiquidToMesh(MesherInfo& mi) {
    return;
    const Block &block = Blocks[mi.btype];
    Block* nextBlock;
    i32 nextBlockID;
    const i32 wc = mi.wc;
    i32 x = mi.x;
    i32 y = mi.y;
    i32 z = mi.z;

    RenderTask* task = mi.task;

    const i32 maxLevel = 100;

    float liquidLevel = block.waterMeshLevel / (float)maxLevel;
    float fallingReduction = 0.0f;

    bool faces[6] = { false, false, false, false, false, false };

    float backLeftHeight = liquidLevel;
    float backRightHeight = liquidLevel;
    float frontRightHeight = liquidLevel;
    float frontLeftHeight = liquidLevel;

    const i32 MIN_ALPHA = 75;
    const i32 MAX_ALPHA = 175;
    const i32 ALPHA_RANGE = MAX_ALPHA - MIN_ALPHA;

    ui8 backRightAlpha, frontRightAlpha, frontLeftAlpha, backLeftAlpha;

    i32 textureUnit = 0;

    i32 div;
    i32 tot;

    i32 left, right, back, front, bottom;

    ui8 uOff = x * 7;
    ui8 vOff = 224 - z * 7;

    ui8 temperature = task->chunkGridData->heightData[x + z*CHUNK_WIDTH].temperature;
    ui8 depth = task->chunkGridData->heightData[x + z*CHUNK_WIDTH].depth;

    ColorRGB8 color = GameManager::texturePackLoader->getColorMap(TerrainGenerator::DefaultColorMaps::WATER)[depth * 256 + temperature];

    ui8 sunlight = mi.task->chSunlightData[wc];
    ColorRGB8 lampLight((mi.task->chLampData[wc] & LAMP_RED_MASK) >> LAMP_RED_SHIFT,
        (mi.task->chLampData[wc] & LAMP_GREEN_MASK) >> LAMP_GREEN_SHIFT,
        mi.task->chLampData[wc] & LAMP_BLUE_MASK);

    sunlight = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - sunlight)));
    lampLight.r = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.r)));
    lampLight.g = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.g)));
    lampLight.b = (ui8)(255.0f*(LIGHT_OFFSET + pow(LIGHT_MULT, MAXLIGHT - lampLight.b)));

    nextBlockID = _blockIDData[wc + PADDED_OFFSETS::BOTTOM];
    nextBlock = &GETBLOCK(nextBlockID);
    //Check if the block is falling
    if (nextBlockID == 0 || nextBlock->waterBreak || (nextBlock->physicsProperty == block.physicsProperty && nextBlock->waterMeshLevel != maxLevel)) {
        memset(faces, 1, 6); //all faces are active
  //      backLeftHeight = backRightHeight = frontLeftHeight = frontRightHeight 
        fallingReduction = 1.0f;
    } else {

        //Get occlusion
        nextBlock = &GETBLOCK(_blockIDData[wc + PADDED_OFFSETS::LEFT]);
        faces[XNEG] = ((nextBlock->physicsProperty != block.physicsProperty) && (nextBlock->occlude == BlockOcclusion::NONE));

        nextBlock = &GETBLOCK(_blockIDData[wc + PADDED_OFFSETS::BACK]);
        faces[ZNEG] = ((nextBlock->physicsProperty != block.physicsProperty) && (nextBlock->occlude == BlockOcclusion::NONE));

        nextBlock = &GETBLOCK(_blockIDData[wc + PADDED_OFFSETS::RIGHT]);
        faces[XPOS] = ((nextBlock->physicsProperty != block.physicsProperty) && (nextBlock->occlude == BlockOcclusion::NONE));

        nextBlock = &GETBLOCK(_blockIDData[wc + PADDED_OFFSETS::FRONT]);
        faces[ZPOS] = ((nextBlock->physicsProperty != block.physicsProperty) && (nextBlock->occlude == BlockOcclusion::NONE));

        nextBlock = &GETBLOCK(_blockIDData[wc + PADDED_OFFSETS::BOTTOM]);
        faces[YNEG] = ((nextBlock->physicsProperty != block.physicsProperty) && (nextBlock->occlude == BlockOcclusion::NONE));
    }

    left = LEVEL(wc + PADDED_OFFSETS::LEFT);
    right = LEVEL(wc + PADDED_OFFSETS::RIGHT);
    back = LEVEL(wc + PADDED_OFFSETS::BACK);
    front = LEVEL(wc + PADDED_OFFSETS::FRONT);
    bottom = LEVEL(wc + PADDED_OFFSETS::BOTTOM);

    //Calculate the liquid levels

    //Back Left Vertex
    CALCULATE_LIQUID_VERTEX_HEIGHT(backLeftHeight, left, back, wc + PADDED_OFFSETS::BACK_LEFT);

    //Back Right Vertex
    CALCULATE_LIQUID_VERTEX_HEIGHT(backRightHeight, right, back, wc + PADDED_OFFSETS::BACK_RIGHT);

    //Front Right Vertex
    CALCULATE_LIQUID_VERTEX_HEIGHT(frontRightHeight, right, front, wc + PADDED_OFFSETS::FRONT_RIGHT);

    //Front Left Vertex
    CALCULATE_LIQUID_VERTEX_HEIGHT(frontLeftHeight, left, front, wc + PADDED_OFFSETS::FRONT_LEFT);

    //only occlude top if we are a full water block and our sides arent down at all
    if (liquidLevel == 1.0f && backRightHeight == 1.0f && backLeftHeight == 1.0f && frontLeftHeight == 1.0f && frontRightHeight == 1.0f) {
        nextBlock = &GETBLOCK(task->chData[wc + PADDED_OFFSETS::TOP]);
        faces[YPOS] = ((nextBlock->physicsProperty != block.physicsProperty) && (nextBlock->occlude == BlockOcclusion::NONE));
    } else {
        faces[YPOS] = true;
    }
    
    //Compute alpha
    if (bottom == maxLevel) {
        backRightAlpha = backLeftAlpha = frontRightAlpha = frontLeftAlpha = MAX_ALPHA;
    } else {
        backRightAlpha = backRightHeight * ALPHA_RANGE + MIN_ALPHA;
        backLeftAlpha = backLeftHeight * ALPHA_RANGE + MIN_ALPHA;
        frontRightAlpha = frontRightHeight * ALPHA_RANGE + MIN_ALPHA;
        frontLeftAlpha = frontLeftHeight * ALPHA_RANGE + MIN_ALPHA;
    }

    //Add vertices for the faces
    if (faces[YNEG]){

        VoxelMesher::makeLiquidFace(_waterVboVerts, mi.liquidIndex, uOff, vOff, lampLight, sunlight, color, textureUnit);
        
        _waterVboVerts[mi.liquidIndex].position[0] = x + VoxelMesher::liquidVertices[48];
        _waterVboVerts[mi.liquidIndex].position[1] = y + VoxelMesher::liquidVertices[49] - fallingReduction;
        _waterVboVerts[mi.liquidIndex].position[2] = z + VoxelMesher::liquidVertices[50];
        _waterVboVerts[mi.liquidIndex + 1].position[0] = x + VoxelMesher::liquidVertices[51];
        _waterVboVerts[mi.liquidIndex + 1].position[1] = y + VoxelMesher::liquidVertices[52] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 1].position[2] = z + VoxelMesher::liquidVertices[53];
        _waterVboVerts[mi.liquidIndex + 2].position[0] = x + VoxelMesher::liquidVertices[54];
        _waterVboVerts[mi.liquidIndex + 2].position[1] = y + VoxelMesher::liquidVertices[55] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 2].position[2] = z + VoxelMesher::liquidVertices[56];
        _waterVboVerts[mi.liquidIndex + 3].position[0] = x + VoxelMesher::liquidVertices[57];
        _waterVboVerts[mi.liquidIndex + 3].position[1] = y + VoxelMesher::liquidVertices[58] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 3].position[2] = z + VoxelMesher::liquidVertices[59];

        //set alpha
        _waterVboVerts[mi.liquidIndex].color.a = backLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 1].color.a = backRightAlpha;
        _waterVboVerts[mi.liquidIndex + 2].color.a = frontRightAlpha;
        _waterVboVerts[mi.liquidIndex + 3].color.a = backLeftAlpha;

        mi.liquidIndex += 4;
    }

    if (faces[ZPOS]){

        VoxelMesher::makeLiquidFace(_waterVboVerts, mi.liquidIndex, uOff, vOff, lampLight, sunlight, color, textureUnit);

        _waterVboVerts[mi.liquidIndex].position[0] = x + VoxelMesher::liquidVertices[0];
        _waterVboVerts[mi.liquidIndex].position[1] = y + frontLeftHeight;
        _waterVboVerts[mi.liquidIndex].position[2] = z + VoxelMesher::liquidVertices[2];
        _waterVboVerts[mi.liquidIndex + 1].position[0] = x + VoxelMesher::liquidVertices[3];
        _waterVboVerts[mi.liquidIndex + 1].position[1] = y + VoxelMesher::liquidVertices[4] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 1].position[2] = z + VoxelMesher::liquidVertices[5];
        _waterVboVerts[mi.liquidIndex + 2].position[0] = x + VoxelMesher::liquidVertices[6];
        _waterVboVerts[mi.liquidIndex + 2].position[1] = y + VoxelMesher::liquidVertices[7] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 2].position[2] = z + VoxelMesher::liquidVertices[8];
        _waterVboVerts[mi.liquidIndex + 3].position[0] = x + VoxelMesher::liquidVertices[9];
        _waterVboVerts[mi.liquidIndex + 3].position[1] = y + frontRightHeight;
        _waterVboVerts[mi.liquidIndex + 3].position[2] = z + VoxelMesher::liquidVertices[11];

        _waterVboVerts[mi.liquidIndex].color.a = frontLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 1].color.a = frontLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 2].color.a = frontRightAlpha;
        _waterVboVerts[mi.liquidIndex + 3].color.a = frontRightAlpha;

        mi.liquidIndex += 4;
    }

    if (faces[YPOS]){

        VoxelMesher::makeLiquidFace(_waterVboVerts, mi.liquidIndex, uOff, vOff, lampLight, sunlight, color, textureUnit);

        _waterVboVerts.resize(_waterVboVerts.size() + 4);
        _waterVboVerts[mi.liquidIndex].position[0] = x + VoxelMesher::liquidVertices[24];
        _waterVboVerts[mi.liquidIndex].position[1] = y + backLeftHeight;
        _waterVboVerts[mi.liquidIndex].position[2] = z + VoxelMesher::liquidVertices[26];
        _waterVboVerts[mi.liquidIndex + 1].position[0] = x + VoxelMesher::liquidVertices[27];
        _waterVboVerts[mi.liquidIndex + 1].position[1] = y + frontLeftHeight;
        _waterVboVerts[mi.liquidIndex + 1].position[2] = z + VoxelMesher::liquidVertices[29];
        _waterVboVerts[mi.liquidIndex + 2].position[0] = x + VoxelMesher::liquidVertices[30];
        _waterVboVerts[mi.liquidIndex + 2].position[1] = y + frontRightHeight;
        _waterVboVerts[mi.liquidIndex + 2].position[2] = z + VoxelMesher::liquidVertices[32];
        _waterVboVerts[mi.liquidIndex + 3].position[0] = x + VoxelMesher::liquidVertices[33];
        _waterVboVerts[mi.liquidIndex + 3].position[1] = y + backRightHeight;
        _waterVboVerts[mi.liquidIndex + 3].position[2] = z + VoxelMesher::liquidVertices[35];

        _waterVboVerts[mi.liquidIndex].color.a = backLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 1].color.a = frontLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 2].color.a = frontRightAlpha;
        _waterVboVerts[mi.liquidIndex + 3].color.a = backRightAlpha;

        mi.liquidIndex += 4;
    }

    if (faces[ZNEG]){

        VoxelMesher::makeLiquidFace(_waterVboVerts, mi.liquidIndex, uOff, vOff, lampLight, sunlight, color, textureUnit);

        _waterVboVerts[mi.liquidIndex].position[0] = x + VoxelMesher::liquidVertices[60];
        _waterVboVerts[mi.liquidIndex].position[1] = y + backRightHeight;
        _waterVboVerts[mi.liquidIndex].position[2] = z + VoxelMesher::liquidVertices[62];
        _waterVboVerts[mi.liquidIndex + 1].position[0] = x + VoxelMesher::liquidVertices[63];
        _waterVboVerts[mi.liquidIndex + 1].position[1] = y + VoxelMesher::liquidVertices[64] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 1].position[2] = z + VoxelMesher::liquidVertices[65];
        _waterVboVerts[mi.liquidIndex + 2].position[0] = x + VoxelMesher::liquidVertices[66];
        _waterVboVerts[mi.liquidIndex + 2].position[1] = y + VoxelMesher::liquidVertices[67] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 2].position[2] = z + VoxelMesher::liquidVertices[68];
        _waterVboVerts[mi.liquidIndex + 3].position[0] = x + VoxelMesher::liquidVertices[69];
        _waterVboVerts[mi.liquidIndex + 3].position[1] = y + backLeftHeight;
        _waterVboVerts[mi.liquidIndex + 3].position[2] = z + VoxelMesher::liquidVertices[71];

        _waterVboVerts[mi.liquidIndex].color.a = backRightAlpha;
        _waterVboVerts[mi.liquidIndex + 1].color.a = backRightAlpha;
        _waterVboVerts[mi.liquidIndex + 2].color.a = backLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 3].color.a = backLeftAlpha;

        mi.liquidIndex += 4;
    }
    if (faces[XPOS]){

        VoxelMesher::makeLiquidFace(_waterVboVerts, mi.liquidIndex, uOff, vOff, lampLight, sunlight, color, textureUnit);

        _waterVboVerts[mi.liquidIndex].position.x = x + VoxelMesher::liquidVertices[12];
        _waterVboVerts[mi.liquidIndex].position.y = y + frontRightHeight;
        _waterVboVerts[mi.liquidIndex].position.z = z + VoxelMesher::liquidVertices[14];
        _waterVboVerts[mi.liquidIndex + 1].position.x = x + VoxelMesher::liquidVertices[15];
        _waterVboVerts[mi.liquidIndex + 1].position.y = y + VoxelMesher::liquidVertices[16] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 1].position.z = z + VoxelMesher::liquidVertices[17];
        _waterVboVerts[mi.liquidIndex + 2].position.x = x + VoxelMesher::liquidVertices[18];
        _waterVboVerts[mi.liquidIndex + 2].position.y = y + VoxelMesher::liquidVertices[19] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 2].position.z = z + VoxelMesher::liquidVertices[20];
        _waterVboVerts[mi.liquidIndex + 3].position.x = x + VoxelMesher::liquidVertices[21];
        _waterVboVerts[mi.liquidIndex + 3].position.y = y + backRightHeight;
        _waterVboVerts[mi.liquidIndex + 3].position.z = z + VoxelMesher::liquidVertices[23];

        _waterVboVerts[mi.liquidIndex].color.a = frontRightAlpha;
        _waterVboVerts[mi.liquidIndex + 1].color.a = frontRightAlpha;
        _waterVboVerts[mi.liquidIndex + 2].color.a = backRightAlpha;
        _waterVboVerts[mi.liquidIndex + 3].color.a = backRightAlpha;

        mi.liquidIndex += 4;
    }
    if (faces[XNEG]){

        VoxelMesher::makeLiquidFace(_waterVboVerts, mi.liquidIndex, uOff, vOff, lampLight, sunlight, color, textureUnit);

        _waterVboVerts[mi.liquidIndex].position[0] = x + VoxelMesher::liquidVertices[36];
        _waterVboVerts[mi.liquidIndex].position[1] = y + backLeftHeight;
        _waterVboVerts[mi.liquidIndex].position[2] = z + VoxelMesher::liquidVertices[38];
        _waterVboVerts[mi.liquidIndex + 1].position[0] = x + VoxelMesher::liquidVertices[39];
        _waterVboVerts[mi.liquidIndex + 1].position[1] = y + VoxelMesher::liquidVertices[40] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 1].position[2] = z + VoxelMesher::liquidVertices[41];
        _waterVboVerts[mi.liquidIndex + 2].position[0] = x + VoxelMesher::liquidVertices[42];
        _waterVboVerts[mi.liquidIndex + 2].position[1] = y + VoxelMesher::liquidVertices[43] - fallingReduction;
        _waterVboVerts[mi.liquidIndex + 2].position[2] = z + VoxelMesher::liquidVertices[44];
        _waterVboVerts[mi.liquidIndex + 3].position[0] = x + VoxelMesher::liquidVertices[45];
        _waterVboVerts[mi.liquidIndex + 3].position[1] = y + frontLeftHeight;
        _waterVboVerts[mi.liquidIndex + 3].position[2] = z + VoxelMesher::liquidVertices[47];

        _waterVboVerts[mi.liquidIndex].color.a = backLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 1].color.a = backLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 2].color.a = frontLeftAlpha;
        _waterVboVerts[mi.liquidIndex + 3].color.a = frontLeftAlpha;

        mi.liquidIndex += 4;
    }
}

void ChunkMesher::mergeTopVerts(MesherInfo &mi)
{
    if (mi.topIndex == 0) return;

    int qi;

    if (_finalTopVerts.size() != 131072) _finalTopVerts.resize(131072);

    for (int i = 0; i < mi.topIndex; i += 4) {
        // Early exit if it cant merge
        if (_topVerts[i].merge == -1) {
            continue;
        } else {
            //Use this quad in the final mesh
            qi = mi.pyVboSize;
            _finalTopVerts[mi.pyVboSize++] = _topVerts[i];
            _finalTopVerts[mi.pyVboSize++] = _topVerts[i + 1];
            _finalTopVerts[mi.pyVboSize++] = _topVerts[i + 2];
            _finalTopVerts[mi.pyVboSize++] = _topVerts[i + 3];
        }
        if (_finalTopVerts[qi].merge != 0 && CompareVerticesLight(_finalTopVerts[qi], _finalTopVerts[qi + 1]) && CompareVerticesLight(_finalTopVerts[qi + 2], _finalTopVerts[qi + 3])) {
            // Search for the next mergeable quad
            for (int j = i + 4; j < mi.topIndex; j += 4) {
                // Skip if the J index is not active for merging
                if (_topVerts[j].merge < 1) continue;

                // Early exit if the J index cant be merged
                if (CompareVerticesLight(_topVerts[j], _topVerts[j + 1]) && CompareVerticesLight(_topVerts[j + 2], _topVerts[j + 3])) {
                    // Early exit if we went too far
                    if (_topVerts[j+1].position.z > _finalTopVerts[qi+1].position.z + 7) break;

                    // If the x locations of the quads line up
                    if (_finalTopVerts[qi].position.x == _topVerts[j].position.x && _finalTopVerts[qi+2].position.x == _topVerts[j+2].position.x) {
                        if (CompareVertices(_finalTopVerts[qi + 1], _topVerts[j]) && CompareVertices(_finalTopVerts[qi + 2], _topVerts[j + 3])) {
                            //they are stretchable, so strech the current quad
                            _finalTopVerts[qi + 1].position.z += 7;
                            _finalTopVerts[qi + 2].position.z += 7;
                            _finalTopVerts[qi + 1].tex[1]--;
                            _finalTopVerts[qi + 2].tex[1]--;
                            //indicate that we want to ignore
                            _topVerts[j].merge = -1;
                        } else{
                            //Else, that was our only shot. Next quad!
                            break;
                        }
                    }
                } else {
                    //Indicate that its not mergeable
                    _topVerts[j].merge = 0;
                }
            }
        }
    }
}

void ChunkMesher::mergeBackVerts(MesherInfo &mi)
{
    //***********************back Y merging
    if (mi.backIndex){
        //Double buffered prev list to avoid a copy
        int *prevQuads = _prevBackQuads[_currPrevBackQuads];
        if (_currPrevBackQuads == 1) {
            _currPrevBackQuads = 0;
        } else {
            _currPrevBackQuads = 1;
        }
        int qj;
        //Make sure we have a buffer allocated (this is stupid)
        if (_finalBackVerts.size() != 131072) _finalBackVerts.resize(131072);

        if (mi.pLayerBackIndex != 0) {
            //Store the size of prev layer and zero the index since we double buffer
            int pLayerBackIndex = mi.pLayerBackIndex;
            mi.pLayerBackIndex = 0;
            //Iterate again for upward merging
            for (int i = 0; i < mi.backIndex; i += 4) {
                //check if its upward mergeable
                if (CompareVerticesLight(_backVerts[i], _backVerts[i + 1]) && CompareVerticesLight(_backVerts[i + 2], _backVerts[i + 3])) {
                    //indicate that it can upward merge
                    _backVerts[i].merge = 1;
                    for (int j = 0; j < pLayerBackIndex; j++){
                        qj = prevQuads[j];
                        if (_finalBackVerts[qj + 2].position.z >= _backVerts[i + 2].position.z) {
                            //Early exit if we went too far
                            if (_finalBackVerts[qj + 2].position.z > _backVerts[i + 2].position.z) break;
                            //Make sure its lined up
                            if (_finalBackVerts[qj + 2].position.x == _backVerts[i + 2].position.x && _finalBackVerts[qj].position.x == _backVerts[i].position.x) {
                                if (CompareVertices(_backVerts[i + 1], _finalBackVerts[qj]) && CompareVertices(_backVerts[i + 2], _finalBackVerts[qj + 3])) {
                                    //They can stretch!
                                    _finalBackVerts[qj].position.y += 7;
                                    _finalBackVerts[qj + 3].position.y += 7;
                                    _finalBackVerts[qj].tex[1]++;
                                    _finalBackVerts[qj + 3].tex[1]++;
                                    _prevBackQuads[_currPrevBackQuads][mi.pLayerBackIndex++] = qj;
                                    _backVerts[i].merge = -1;
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    //signal that it is not upward mergeable
                    _backVerts[i].merge = 0;
                }
                //If the vertex still is active, add to mesh
                if (_backVerts[i].merge != -1) {

                    //if it is upward mergeable, then add to prev back verts
                    if (_backVerts[i].merge > 0) {
                        _prevBackQuads[_currPrevBackQuads][mi.pLayerBackIndex++] = mi.nzVboSize;
                    }

                    _finalBackVerts[mi.nzVboSize++] = _backVerts[i];
                    _finalBackVerts[mi.nzVboSize++] = _backVerts[i + 1];
                    _finalBackVerts[mi.nzVboSize++] = _backVerts[i + 2];
                    _finalBackVerts[mi.nzVboSize++] = _backVerts[i + 3];

                }
            }

        } else { //if there was no previous layer, add verts to final mesh
            //we can assume merge is != -1.
            for (int i = 0; i < mi.backIndex; i+=4) {
              
                //if it is upward mergeable, then add to prev back verts
                if (_backVerts[i].merge > 0 && CompareVertices(_backVerts[i], _backVerts[i + 1]) && CompareVertices(_backVerts[i + 2], _backVerts[i + 3])) {
                    _prevBackQuads[_currPrevBackQuads][mi.pLayerBackIndex++] = mi.nzVboSize;
                }

                _finalBackVerts[mi.nzVboSize++] = _backVerts[i];
                _finalBackVerts[mi.nzVboSize++] = _backVerts[i + 1];
                _finalBackVerts[mi.nzVboSize++] = _backVerts[i + 2];
                _finalBackVerts[mi.nzVboSize++] = _backVerts[i + 3];
            }
        }
    }
    else{
        mi.pLayerBackIndex = 0;
    }
}

void ChunkMesher::mergeFrontVerts(MesherInfo &mi)
{
    //***********************front Y merging
    if (mi.frontIndex){
        //Double buffered prev list to avoid a copy
        int *prevQuads = _prevFrontQuads[_currPrevFrontQuads];
        if (_currPrevFrontQuads == 1) {
            _currPrevFrontQuads = 0;
        } else {
            _currPrevFrontQuads = 1;
        }
        int qj;
        //Make sure we have a buffer allocated (this is stupid)
        if (_finalFrontVerts.size() != 131072) _finalFrontVerts.resize(131072);

        if (mi.pLayerFrontIndex != 0) {
            //Store the size of prev layer and zero the index since we double buffer
            int pLayerFrontIndex = mi.pLayerFrontIndex;
            mi.pLayerFrontIndex = 0;
            //Iterate again for upward merging
            for (int i = 0; i < mi.frontIndex; i += 4) {
                //check if its upward mergeable
                if (CompareVerticesLight(_frontVerts[i], _frontVerts[i + 1]) && CompareVerticesLight(_frontVerts[i + 2], _frontVerts[i + 3])) {
                    //indicate that it can upward merge
                    _frontVerts[i].merge = 1;
                    for (int j = 0; j < pLayerFrontIndex; j++){
                        qj = prevQuads[j];
                        if (_finalFrontVerts[qj + 2].position.z >= _frontVerts[i + 2].position.z) {
                            //Early exit if we went too far
                            if (_finalFrontVerts[qj + 2].position.z > _frontVerts[i + 2].position.z) break;
                            //Make sure its lined up
                            if (_finalFrontVerts[qj + 2].position.x == _frontVerts[i + 2].position.x && _finalFrontVerts[qj].position.x == _frontVerts[i].position.x) {
                                if (CompareVertices(_frontVerts[i + 1], _finalFrontVerts[qj]) && CompareVertices(_frontVerts[i + 2], _finalFrontVerts[qj + 3])) {
                                    //They can stretch!
                                    _finalFrontVerts[qj].position.y += 7;
                                    _finalFrontVerts[qj + 3].position.y += 7;
                                    _finalFrontVerts[qj].tex[1]++;
                                    _finalFrontVerts[qj + 3].tex[1]++;
                                    _prevFrontQuads[_currPrevFrontQuads][mi.pLayerFrontIndex++] = qj;
                                    _frontVerts[i].merge = -1;
                                    break;
                                }
                            }
                        }
                    }
                } else {
                    //signal that it is not upward mergeable
                    _frontVerts[i].merge = 0;
                }
                //If the vertex still is active, add to mesh
                if (_frontVerts[i].merge != -1) {

                    //if it is upward mergeable, then add to prev front verts
                    if (_frontVerts[i].merge > 0) {
                        _prevFrontQuads[_currPrevFrontQuads][mi.pLayerFrontIndex++] = mi.pzVboSize;
                    }

                    _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i];
                    _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i + 1];
                    _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i + 2];
                    _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i + 3];

                }
            }

        } else { //if there was no previous layer, add verts to final mesh
            //we can assume merge is != -1.
            for (int i = 0; i < mi.frontIndex; i += 4) {

                //if it is upward mergeable, then add to prev front verts
                if (_frontVerts[i].merge > 0 && CompareVertices(_frontVerts[i], _frontVerts[i + 1]) && CompareVertices(_frontVerts[i + 2], _frontVerts[i + 3])) {
                    _prevFrontQuads[_currPrevFrontQuads][mi.pLayerFrontIndex++] = mi.pzVboSize;
                }

                _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i];
                _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i + 1];
                _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i + 2];
                _finalFrontVerts[mi.pzVboSize++] = _frontVerts[i + 3];
            }
        }
    } else{
        mi.pLayerFrontIndex = 0;
    }
}

void ChunkMesher::mergeRightVerts(MesherInfo &mi) {

    if (mi.rightIndex == 0){
        mi.pLayerRightIndex = 0;
        return;
    }
    int finalQuadIndex = 0;

// Loop through each quad
    for (int i = 0; i < mi.rightIndex; i+=4) {
        // Early exit if it cant merge
        if (_rightVerts[i].merge == -1) {
            continue;
        } else {
            //Probably use this quad in the final mesh
            _finalQuads[finalQuadIndex++] = i;
        }
        if (_rightVerts[i].merge != 0 && CompareVerticesLight(_rightVerts[i], _rightVerts[i + 3]) && CompareVerticesLight(_rightVerts[i + 1], _rightVerts[i + 2])) {
            // Search for the next mergeable quad
            for (int j = i+4; j < mi.rightIndex; j += 4) {
                // Skip if the J index is not active for merging
                if (_rightVerts[j].merge < 1) continue;
            
                // Early exit if the J index cant be merged
                if (CompareVerticesLight(_rightVerts[j], _rightVerts[j + 3]) && CompareVerticesLight(_rightVerts[j + 1], _rightVerts[j + 2])) {
                    // Early exit if we went too far
                    if (_rightVerts[j].position.z > _rightVerts[i].position.z + 7) break;

                    // If the x locations of the quads line up
                    if (_rightVerts[i].position.x == _rightVerts[j].position.x) {
                        if (CompareVertices(_rightVerts[i], _rightVerts[j + 3]) && CompareVertices(_rightVerts[i + 1], _rightVerts[j + 2])) {
                            //they are stretchable, so strech the current quad
                            _rightVerts[i].position.z += 7;
                            _rightVerts[i + 1].position.z += 7;
                            _rightVerts[i].tex[0]--;
                            _rightVerts[i + 1].tex[0]--;
                            //indicate that we want to ignore
                            _rightVerts[j].merge = -1;
                        } else{
                            //Else, that was our only shot. Next quad!
                            break;
                        }
                    }
                } else {
                    //Indicate that its not mergeable
                    _rightVerts[j].merge = 0;
                }
            }
        }
    }

    int qi;
    int qj;

    //Double buffered prev list to avoid a copy
    int *prevQuads = _prevRightQuads[_currPrevRightQuads];
    if (_currPrevRightQuads == 1) {
        _currPrevRightQuads = 0;
    } else {
        _currPrevRightQuads = 1;
    }

    //Make sure we have a buffer allocated (this is stupid)
    if (_finalRightVerts.size() != 131072) _finalRightVerts.resize(131072);

    if (mi.pLayerRightIndex != 0) {
        //Store the size of prev layer and zero the index since we double buffer
        int pLayerRightIndex = mi.pLayerRightIndex;
        mi.pLayerRightIndex = 0;
        //Iterate again for upward merging
        for (int i = 0; i < finalQuadIndex; i++) {
            qi = _finalQuads[i];
            //check if its upward mergeable
            if (CompareVerticesLight(_rightVerts[qi], _rightVerts[qi + 1]) && CompareVerticesLight(_rightVerts[qi + 2], _rightVerts[qi + 3])) {
                //indicate that it can upward merge
                _rightVerts[qi].merge = 1;
                for (int j = 0; j < pLayerRightIndex; j++){
                    qj = prevQuads[j];
                    if (_finalRightVerts[qj + 2].position.z >= _rightVerts[qi + 2].position.z) {
                        //Early exit if we went too far
                        if (_finalRightVerts[qj + 2].position.z > _rightVerts[qi + 2].position.z) break;
                        //Make sure its lined up
                        if (_finalRightVerts[qj].position.z == _rightVerts[qi].position.z && _finalRightVerts[qj].position.x == _rightVerts[qi].position.x) {
                            if (CompareVertices(_rightVerts[qi + 1], _finalRightVerts[qj]) && CompareVertices(_rightVerts[qi + 2], _finalRightVerts[qj + 3])) {
                                //They can stretch!
                                _finalRightVerts[qj].position.y += 7;
                                _finalRightVerts[qj + 3].position.y += 7;
                                _finalRightVerts[qj].tex[1]++;
                                _finalRightVerts[qj + 3].tex[1]++;
                                _prevRightQuads[_currPrevRightQuads][mi.pLayerRightIndex++] = qj;
                                _rightVerts[qi].merge = -1;
                                break;
                            }
                        }
                    }
                }
            } else {
                //signal that it is not upward mergeable
                _rightVerts[qi].merge = 0;
            }
            //If the vertex still is active, add to mesh
            if (_rightVerts[qi].merge != -1) {

                //if it is upward mergeable, then add to prev right verts
                if (_rightVerts[qi].merge > 0) {
                    _prevRightQuads[_currPrevRightQuads][mi.pLayerRightIndex++] = mi.pxVboSize;
                }

                _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi];
                _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi + 1];
                _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi + 2];
                _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi + 3];
               
            }
        }

    } else { //if there was no previous layer, add verts to final mesh
        //we can assume merge is != -1.
        for (int i = 0; i < finalQuadIndex; i++) {
            qi = _finalQuads[i];

            //if it is upward mergeable, then add to prev right verts
            if (_rightVerts[qi].merge > 0 && CompareVertices(_rightVerts[qi], _rightVerts[qi + 1]) && CompareVertices(_rightVerts[qi + 2], _rightVerts[qi + 3])) {
                _prevRightQuads[_currPrevRightQuads][mi.pLayerRightIndex++] = mi.pxVboSize;
            }

            _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi];
            _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi + 1];
            _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi + 2];
            _finalRightVerts[mi.pxVboSize++] = _rightVerts[qi + 3];
        }
    }
}

void ChunkMesher::mergeLeftVerts(MesherInfo &mi)
{
    if (mi.leftIndex == 0) {
        mi.pLayerLeftIndex = 0;
        return;
    }
    int finalQuadIndex = 0;

    // Loop through each quad
    for (int i = 0; i < mi.leftIndex; i += 4) {
        // Early exit if it cant merge
        if (_leftVerts[i].merge == -1) {
            continue;
        } else {
            //Probably use this quad in the final mesh
            _finalQuads[finalQuadIndex++] = i;
        }
        if (_leftVerts[i].merge != 0 && CompareVerticesLight(_leftVerts[i], _leftVerts[i + 3]) && CompareVerticesLight(_leftVerts[i + 1], _leftVerts[i + 2])) {
            // Search for the next mergeable quad
            for (int j = i + 4; j < mi.leftIndex; j += 4) {
                // Skip if the J index is not active for merging
                if (_leftVerts[j].merge < 1) continue;

                // Early exit if the J index cant be merged
                if (CompareVerticesLight(_leftVerts[j], _leftVerts[j + 3]) && CompareVerticesLight(_leftVerts[j + 1], _leftVerts[j + 2])) {
                    // Early exit if we went too far
                    if (_leftVerts[j + 2].position.z > _leftVerts[i + 2].position.z + 7) break;

                    // If the x locations of the quads line up
                    if (_leftVerts[i].position.x == _leftVerts[j].position.x) {
                        if (CompareVertices(_leftVerts[i + 3], _leftVerts[j]) && CompareVertices(_leftVerts[i + 2], _leftVerts[j + 1])) {
                            //they are stretchable, so strech the current quad
                            _leftVerts[i + 2].position.z += 7;
                            _leftVerts[i + 3].position.z += 7;
                            _leftVerts[i + 2].tex[0]++;
                            _leftVerts[i + 3].tex[0]++;
                            //indicate that we want to ignore
                            _leftVerts[j].merge = -1;
                        } else{
                            //Else, that was our only shot. Next quad!
                            break;
                        }
                    }
                } else {
                    //Indicate that its not mergeable
                    _leftVerts[j].merge = 0;
                }
            }
        }
    }

    int qi;
    int qj;

    //Double buffered prev list to avoid a copy
    int *prevQuads = _prevLeftQuads[_currPrevLeftQuads];
    if (_currPrevLeftQuads == 1) {
        _currPrevLeftQuads = 0;
    } else {
        _currPrevLeftQuads = 1;
    }

    //Make sure we have a buffer allocated (this is stupid)
    if (_finalLeftVerts.size() != 131072) _finalLeftVerts.resize(131072);

    if (mi.pLayerLeftIndex != 0) {
        //Store the size of prev layer and zero the index since we double buffer
        int pLayerLeftIndex = mi.pLayerLeftIndex;
        mi.pLayerLeftIndex = 0;
        //Iterate again for upward merging
        for (int i = 0; i < finalQuadIndex; i++) {
            qi = _finalQuads[i];
            //check if its upward mergeable
            if (CompareVerticesLight(_leftVerts[qi], _leftVerts[qi + 1]) && CompareVerticesLight(_leftVerts[qi + 2], _leftVerts[qi + 3])) {
                //indicate that it can upward merge
                _leftVerts[qi].merge = 1;
                for (int j = 0; j < pLayerLeftIndex; j++){
                    qj = prevQuads[j];
                    if (_finalLeftVerts[qj].position.z >= _leftVerts[qi].position.z) {
                        //Early exit if we went too far
                        if (_finalLeftVerts[qj].position.z > _leftVerts[qi].position.z) break;
                        //Make sure its lined up
                        if (_finalLeftVerts[qj+2].position.z == _leftVerts[qi+2].position.z && _finalLeftVerts[qj].position.x == _leftVerts[qi].position.x) {
                            if (CompareVertices(_leftVerts[qi + 1], _finalLeftVerts[qj]) && CompareVertices(_leftVerts[qi + 2], _finalLeftVerts[qj + 3])) {
                                //They can stretch!
                                _finalLeftVerts[qj].position.y += 7;
                                _finalLeftVerts[qj + 3].position.y += 7;
                                _finalLeftVerts[qj].tex[1]++;
                                _finalLeftVerts[qj + 3].tex[1]++;
                                _prevLeftQuads[_currPrevLeftQuads][mi.pLayerLeftIndex++] = qj;
                                _leftVerts[qi].merge = -1;
                                break;
                            }
                        }
                    }
                }
            } else {
                //signal that it is not upward mergeable
                _leftVerts[qi].merge = 0;
            }
            //If the vertex still is active, add to mesh
            if (_leftVerts[qi].merge != -1) {

                //if it is upward mergeable, then add to prev left verts
                if (_leftVerts[qi].merge > 0) {
                    _prevLeftQuads[_currPrevLeftQuads][mi.pLayerLeftIndex++] = mi.nxVboSize;
                }

                _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi];
                _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi + 1];
                _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi + 2];
                _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi + 3];

            }
        }

    } else { //if there was no previous layer, add verts to final mesh
        //we can assume merge is != -1.
        for (int i = 0; i < finalQuadIndex; i++) {
            qi = _finalQuads[i];

            //if it is upward mergeable, then add to prev left verts
            if (_leftVerts[qi].merge > 0 && CompareVertices(_leftVerts[qi], _leftVerts[qi + 1]) && CompareVertices(_leftVerts[qi + 2], _leftVerts[qi + 3])) {
                _prevLeftQuads[_currPrevLeftQuads][mi.pLayerLeftIndex++] = mi.nxVboSize;
            }

            _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi];
            _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi + 1];
            _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi + 2];
            _finalLeftVerts[mi.nxVboSize++] = _leftVerts[qi + 3];
        }
    }
}

void ChunkMesher::mergeBottomVerts(MesherInfo &mi)
{
    if (mi.botIndex == 0) return;

    int qi;

    if (_finalBottomVerts.size() != 131072) _finalBottomVerts.resize(131072);

    for (int i = 0; i < mi.botIndex; i += 4) {
        // Early exit if it cant merge
        if (_bottomVerts[i].merge == -1) {
            continue;
        } else {
            //Use this quad in the final mesh
            qi = mi.nyVboSize;
            _finalBottomVerts[mi.nyVboSize++] = _bottomVerts[i];
            _finalBottomVerts[mi.nyVboSize++] = _bottomVerts[i + 1];
            _finalBottomVerts[mi.nyVboSize++] = _bottomVerts[i + 2];
            _finalBottomVerts[mi.nyVboSize++] = _bottomVerts[i + 3];
        }
        if (_finalBottomVerts[qi].merge != 0 && CompareVerticesLight(_finalBottomVerts[qi], _finalBottomVerts[qi + 1]) && CompareVerticesLight(_finalBottomVerts[qi + 2], _finalBottomVerts[qi + 3])) {
            // Search for the next mergeable quad
            for (int j = i + 4; j < mi.botIndex; j += 4) {
                // Skip if the J index is not active for merging
                if (_bottomVerts[j].merge < 1) continue;

                // Early exit if the J index cant be merged
                if (CompareVerticesLight(_bottomVerts[j], _bottomVerts[j + 1]) && CompareVerticesLight(_bottomVerts[j + 2], _bottomVerts[j + 3])) {
                    // Early exit if we went too far
                    if (_bottomVerts[j + 1].position.z > _finalBottomVerts[qi + 1].position.z + 7) break;

                    // If the x locations of the quads line up
                    if (_finalBottomVerts[qi].position.x == _bottomVerts[j].position.x && _finalBottomVerts[qi + 2].position.x == _bottomVerts[j + 2].position.x) {
                        if (CompareVertices(_finalBottomVerts[qi + 1], _bottomVerts[j]) && CompareVertices(_finalBottomVerts[qi + 2], _bottomVerts[j + 3])) {
                            //they are stretchable, so strech the current quad
                            _finalBottomVerts[qi + 1].position.z += 7;
                            _finalBottomVerts[qi + 2].position.z += 7;
                            _finalBottomVerts[qi + 1].tex[1]--;
                            _finalBottomVerts[qi + 2].tex[1]--;
                            //indicate that we want to ignore
                            _bottomVerts[j].merge = -1;
                        } else{
                            //Else, that was our only shot. Next quad!
                            break;
                        }
                    }
                } else {
                    //Indicate that its not mergeable
                    _bottomVerts[j].merge = 0;
                }
            }
        }
    }
}

bool ChunkMesher::createChunkMesh(RenderTask *renderTask)
{

    int waveEffect;
    Block *block;

    //Stores the information about the current mesh job
    MesherInfo mi = {};

    _waterVboVerts.clear();
    _transparentVerts.clear();
    _cutoutVerts.clear();

    //store the render task so we can pass it to functions
    mi.task = renderTask;
    mi.chunkGridData = renderTask->chunkGridData;

    //Used in merging
    _currPrevRightQuads = 0;
    _currPrevLeftQuads = 0;
    _currPrevBackQuads = 0;
    _currPrevFrontQuads = 0;

    //create a new chunk mesh data container
    if (chunkMeshData != NULL){
        ERROR("Tried to create mesh with in use chunkMeshData!");
        return 0;
    }

    //Stores the data for a chunk mesh
    chunkMeshData = new ChunkMeshData(renderTask);

    mi.blockIDData = _blockIDData = renderTask->chData;
    mi.lampLightData = _lampLightData = renderTask->chLampData;
    mi.sunlightData = _sunlightData = renderTask->chSunlightData;
    mi.tertiaryData = _tertiaryData = renderTask->chTertiaryData;



    if (renderTask->levelOfDetail > 1) {
        computeLODData(renderTask->levelOfDetail);
    } else {
        dataLayer = PADDED_CHUNK_LAYER;
        dataWidth = PADDED_CHUNK_WIDTH;
        dataSize = PADDED_CHUNK_SIZE;
    }

    // Init the mesh info
    mi.init(dataWidth, dataLayer);

    mi.levelOfDetail = renderTask->levelOfDetail;
    int lodStep = (1 << (mi.levelOfDetail - 1));

    for (mi.y = 0; mi.y < dataWidth-2; mi.y++) {

        mi.y2 = mi.y * 2;
        mi.ny = mi.y * lodStep;

        //reset all indexes to zero for each layer
        mi.topIndex = 0;
        mi.leftIndex = 0;
        mi.rightIndex = 0;
        mi.frontIndex = 0;
        mi.backIndex = 0;
        mi.botIndex = 0;
        for (mi.z = 0; mi.z < dataWidth-2; mi.z++){

            mi.z2 = mi.z * 2;
            mi.nz = mi.z * lodStep;

            for (mi.x = 0; mi.x < dataWidth-2; mi.x++){

                mi.x2 = mi.x * 2;
                mi.nx = mi.x * lodStep;
 
                //We use wc instead of c, because the array is sentinalized at all edges so we dont have to access neighbor chunks with mutexes
                mi.wc = (mi.y + 1)*(dataLayer)+(mi.z + 1)*(dataWidth)+(mi.x + 1); //get the expanded c for our sentinelized array
                //get the block properties
                mi.btype = GETBLOCKTYPE(_blockIDData[mi.wc]);
                block = &(Blocks[mi.btype]);


                waveEffect = block->waveEffect;
                //water is currently hardcoded 
                if (mi.btype >= LOWWATER){
                    addLiquidToMesh(mi);
                } else {
                    //Check the mesh type
                    mi.meshType = Blocks[mi.btype].meshType;

                    switch (mi.meshType){
                        case MeshType::BLOCK:
                            addBlockToMesh(mi);
                            break;
                        case MeshType::LEAVES:
                        case MeshType::CROSSFLORA:
                        case MeshType::FLORA:
                            addFloraToMesh(mi);
                            break;
                        default:
                            //No mesh, do nothing
                            break;
                    }

                }
                //Store the current block for use next frame as the previous block
                mi.pbtype = mi.btype;
            }
            //We reset +x merging for up, front, back, bottom each z iteration
            mi.mergeUp = 0;
            mi.mergeFront = 0;
            mi.mergeBack = 0;
            mi.mergeBot = 0;
        }
        //second pass of merging for all directions
        mergeTopVerts(mi);
        mergeFrontVerts(mi);
        mergeBackVerts(mi);
        mergeRightVerts(mi);
        mergeLeftVerts(mi);
        mergeBottomVerts(mi);
    }

    int highestY = 0, lowestY = 256, highestX = 0, lowestX = 256, highestZ = 0, lowestZ = 256;
    int pyVboOff, pxVboOff, pzVboOff, nyVboOff, nxVboOff, nzVboOff;
    int index = 0;
    pyVboOff = 0;

    chunkMeshData->vertices.resize(mi.pyVboSize + mi.nxVboSize + mi.pxVboSize + mi.nzVboSize + mi.pzVboSize + mi.nyVboSize);
    for (int i = 0; i < mi.pyVboSize; i++){
        if (_finalTopVerts[i].position.y < lowestY) lowestY = _finalTopVerts[i].position.y;
        chunkMeshData->vertices[index++] = _finalTopVerts[i];
    }
    nxVboOff = index;
    for (int i = 0; i < mi.nxVboSize; i++){
        if (_finalLeftVerts[i].position.x > highestX) highestX = _finalLeftVerts[i].position.x;
        chunkMeshData->vertices[index++] = _finalLeftVerts[i];
    }
    pxVboOff = index;
    for (int i = 0; i < mi.pxVboSize; i++){
        if (_finalRightVerts[i].position.x < lowestX) lowestX = _finalRightVerts[i].position.x;
        chunkMeshData->vertices[index++] = _finalRightVerts[i];
    }
    nzVboOff = index;
    for (int i = 0; i < mi.nzVboSize; i++){
        if (_finalBackVerts[i].position.z > highestZ) highestZ = _finalBackVerts[i].position.z;
        chunkMeshData->vertices[index++] = _finalBackVerts[i];
    }
    pzVboOff = index;
    for (int i = 0; i < mi.pzVboSize; i++){
        if (_finalFrontVerts[i].position.z < lowestZ) lowestZ = _finalFrontVerts[i].position.z;
        chunkMeshData->vertices[index++] = _finalFrontVerts[i];
    }
    nyVboOff = index;
    for (int i = 0; i < mi.nyVboSize; i++){
        if (_finalBottomVerts[i].position.y > highestY) highestY = _finalBottomVerts[i].position.y;
        chunkMeshData->vertices[index++] = _finalBottomVerts[i];
    }

    if (mi.transparentIndex) {
        chunkMeshData->transVertices.swap(_transparentVerts);
        assert(chunkMeshData->transQuadIndices.size() != 0);
    }

    if (mi.cutoutIndex) {
        chunkMeshData->cutoutVertices.swap(_cutoutVerts);
    }

    highestY /= 7;
    lowestY /= 7;
    highestX /= 7;
    lowestX /= 7;
    highestZ /= 7;
    lowestZ /= 7;

    int indice = (index / 4) * 6;

    ChunkMeshRenderData& meshInfo = chunkMeshData->meshInfo;

    //add all vertices to the vbo
    if (chunkMeshData->vertices.size() || chunkMeshData->transVertices.size() || chunkMeshData->cutoutVertices.size()) {
        meshInfo.indexSize = (chunkMeshData->vertices.size() * 6) / 4;

		//now let the sizes represent indice sizes
		meshInfo.pyVboOff = pyVboOff;
		meshInfo.pyVboSize = (mi.pyVboSize / 4) * 6;
		meshInfo.nyVboOff = nyVboOff;
		meshInfo.nyVboSize = (mi.nyVboSize / 4) * 6;
		meshInfo.pxVboOff = pxVboOff;
		meshInfo.pxVboSize = (mi.pxVboSize / 4) * 6;
		meshInfo.nxVboOff = nxVboOff;
		meshInfo.nxVboSize = (mi.nxVboSize / 4) * 6;
		meshInfo.pzVboOff = pzVboOff;
		meshInfo.pzVboSize = (mi.pzVboSize / 4) * 6;
		meshInfo.nzVboOff = nzVboOff;
		meshInfo.nzVboSize = (mi.nzVboSize / 4) * 6;

        meshInfo.transVboSize = (mi.transparentIndex / 4) * 6;
        meshInfo.cutoutVboSize = (mi.cutoutIndex / 4) * 6;
      
		meshInfo.highestY = highestY;
		meshInfo.lowestY = lowestY;
		meshInfo.highestX = highestX;
		meshInfo.lowestX = lowestX;
		meshInfo.highestZ = highestZ;
		meshInfo.lowestZ = lowestZ;
	}

	if (mi.liquidIndex){
        meshInfo.waterIndexSize = (mi.liquidIndex * 6) / 4;
        chunkMeshData->waterVertices.swap(_waterVboVerts);
	}
	return 0;
}

bool ChunkMesher::createOnlyWaterMesh(RenderTask *renderTask)
{
	if (chunkMeshData != NULL){
		ERROR("Tried to create mesh with in use chunkMeshData!");
		return 0;
	}
    chunkMeshData = new ChunkMeshData(renderTask);

    _waterVboVerts.clear();
    MesherInfo mi = {};

    mi.task = renderTask;

    for (int i = 0; i < renderTask->wSize; i++) {
        mi.wc = renderTask->wvec[i];
        mi.btype = GETBLOCKTYPE(renderTask->chData[mi.wc]);
        mi.x = (mi.wc % PADDED_CHUNK_WIDTH) - 1;
        mi.y = (mi.wc / PADDED_CHUNK_LAYER) - 1;
        mi.z = ((mi.wc % PADDED_CHUNK_LAYER) / PADDED_CHUNK_WIDTH) - 1;
      
        addLiquidToMesh(mi);
    }


	if (mi.liquidIndex){
        chunkMeshData->meshInfo.waterIndexSize = (mi.liquidIndex * 6) / 4;
		chunkMeshData->waterVertices.swap(_waterVboVerts);
	}
    
    return 0;
}

void ChunkMesher::freeBuffers()
{
    //free memory
    vector <BlockVertex>().swap(_vboVerts);

    //These dont get too big so it might be ok to not free them?
    /*vector <Vertex>().swap(waterVboVerts);
    vector<Vertex>().swap(finalTopVerts);
    vector<Vertex>().swap(finalLeftVerts);
    vector<Vertex>().swap(finalRightVerts);
    vector<Vertex>().swap(finalFrontVerts);
    vector<Vertex>().swap(finalBackVerts);
    vector<Vertex>().swap(finalBottomVerts);
    vector<Vertex>().swap(finalNbVerts);*/
}

void ChunkMesher::computeLODData(int levelOfDetail) {

    int lodStep = 1 << (levelOfDetail - 1);
    dataWidth = CHUNK_WIDTH / lodStep + 2;
    dataLayer = dataWidth * dataWidth;
    dataSize = dataLayer * dataWidth;

    int lodIndex;
    int index;

    //******* Set edge data *******

    //Hardcoded mapping of LOD map to default map
    static const int lodMapping[5][18] = {
        { 0, 1, 3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 23, 25, 27, 29, 31, 33 },
        { 0, 1, 5, 9, 13, 17, 21, 25, 29, 33, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 9, 17, 15, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 17, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
        { 0, 1, 33, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 } };

    const int* lodMap = lodMapping[levelOfDetail-2];

    //bottom edge
    for (int z = 0; z < dataWidth; z++) {
        for (int x = 0; x < dataWidth; x++) {
            index = lodMap[z] * PADDED_CHUNK_WIDTH + lodMap[x];
            lodIndex = z * dataWidth + x;
            lodIDData[lodIndex] = _blockIDData[index];
            lodLampData[lodIndex] = _lampLightData[index];
            lodSunData[lodIndex] = MAXLIGHT;
      /*      for (int z2 = 0; z2 < lodStep; z2++) {
                for (int x2 = 0; x2 < lodStep; x2++) {
                    if (GETBLOCK(blockIDData[index + z2 * PADDED_CHUNK_WIDTH + x2]).meshType != MeshType::BLOCK) {
                        lodIDData[lodIndex] = 0;
                        z2 = lodStep; break;
                    }
                }
            }*/
        }
    }
    //top edge
    for (int z = 0; z < dataWidth; z++) {
        for (int x = 0; x < dataWidth; x++) {
            index = (PADDED_CHUNK_SIZE - PADDED_CHUNK_LAYER) + lodMap[z] * PADDED_CHUNK_WIDTH + lodMap[x];
            lodIndex = (dataSize - dataLayer) + z * dataWidth + x;
            lodIDData[lodIndex] = _blockIDData[index];
            lodLampData[lodIndex] = _lampLightData[index];
            lodSunData[lodIndex] = MAXLIGHT;
          /*  for (int z2 = 0; z2 < lodStep; z2++) {
                for (int x2 = 0; x2 < lodStep; x2++) {
                    if (GETBLOCK(blockIDData[index + z2 * PADDED_CHUNK_WIDTH + x2]).meshType != MeshType::BLOCK) {
                        lodIDData[lodIndex] = 0;
                        z2 = lodStep; break;
                    }
                }
            }*/
        }
    }
    //back edge
    for (int y = 1; y < dataWidth-1; y++) {
        for (int x = 0; x < dataWidth; x++) {
            index = lodMap[y] * PADDED_CHUNK_LAYER + lodMap[x];
            lodIndex = y * dataLayer + x;
            lodIDData[lodIndex] = _blockIDData[index];
            lodLampData[lodIndex] = _lampLightData[index];
            lodSunData[lodIndex] = MAXLIGHT;
            for (int y2 = 0; y2 < lodStep; y2++) {
                for (int x2 = 0; x2 < lodStep; x2++) {
                    if (GETBLOCK(_blockIDData[index + y2 * PADDED_CHUNK_LAYER + x2]).meshType != MeshType::BLOCK) {
                        lodIDData[lodIndex] = 0;
                        y2 = lodStep; break;
                    }
                }
            }
        }
    }
    //front edge
    for (int y = 1; y < dataWidth - 1; y++) {
        for (int x = 0; x < dataWidth; x++) {
            index = (PADDED_CHUNK_LAYER - PADDED_CHUNK_WIDTH) + lodMap[y] * PADDED_CHUNK_LAYER + lodMap[x];
            lodIndex = (dataLayer - dataWidth) + y * dataLayer + x;
            lodIDData[lodIndex] = _blockIDData[index];
            lodLampData[lodIndex] = _lampLightData[index];
            lodSunData[lodIndex] = MAXLIGHT;
            for (int y2 = 0; y2 < lodStep; y2++) {
                for (int x2 = 0; x2 < lodStep; x2++) {
                    if (GETBLOCK(_blockIDData[index + y2 * PADDED_CHUNK_LAYER + x2]).meshType != MeshType::BLOCK) {
                        lodIDData[lodIndex] = 0;
                        y2 = lodStep; break;
                    }
                }
            }
        }
    }
    //left edge
    for (int y = 1; y < dataWidth - 1; y++) {
        for (int z = 1; z < dataWidth - 1; z++) {
            index = lodMap[y] * PADDED_CHUNK_LAYER + lodMap[z] * PADDED_CHUNK_WIDTH;
            lodIndex = y * dataLayer + z * dataWidth;
            lodIDData[lodIndex] = _blockIDData[index];
            lodLampData[lodIndex] = _lampLightData[index];
            lodSunData[lodIndex] = MAXLIGHT;
            for (int y2 = 0; y2 < lodStep; y2++) {
                for (int z2 = 0; z2 < lodStep; z2++) {
                    if (GETBLOCK(_blockIDData[index + y2 * PADDED_CHUNK_LAYER + z2 * PADDED_CHUNK_WIDTH]).meshType != MeshType::BLOCK) {
                        lodIDData[lodIndex] = 0;
                        y2 = lodStep; break;
                    }
                }
            }
        }
    }
    //right edge
    for (int y = 1; y < dataWidth - 1; y++) {
        for (int z = 1; z < dataWidth - 1; z++) {
            index = (PADDED_CHUNK_WIDTH - 1) + lodMap[y] * PADDED_CHUNK_LAYER + lodMap[z] * PADDED_CHUNK_WIDTH;
            lodIndex = (dataWidth - 1) + y * dataLayer + z * dataWidth;
            lodIDData[lodIndex] = _blockIDData[index];
            lodLampData[lodIndex] = _lampLightData[index];
            lodSunData[lodIndex] = MAXLIGHT;
            for (int y2 = 0; y2 < lodStep; y2++) {
                for (int z2 = 0; z2 < lodStep; z2++) {
                    if (GETBLOCK(_blockIDData[index + y2 * PADDED_CHUNK_LAYER + z2 * PADDED_CHUNK_WIDTH]).meshType != MeshType::BLOCK) {
                        lodIDData[lodIndex] = 0;
                        y2 = lodStep; break;
                    }
                }
            }
        }
    }
    vector <ui16> blocks;
    //******* End edge data *******
    for (int y = 1; y < dataWidth - 1; y++) {
        for (int z = 1; z < dataWidth - 1; z++) {
            for (int x = 1; x < dataWidth - 1; x++) {

                for (int z2 = lodMap[z]; z2 < lodMap[z] + lodStep; z2++) {
                    for (int x2 = lodMap[x]; x2 < lodMap[x] + lodStep; x2++) {
                        for (int y2 = lodMap[y] + lodStep - 1; y2 >= lodMap[y]; y2--) {
                            index = y2 * PADDED_CHUNK_LAYER + z2 * PADDED_CHUNK_WIDTH + x2;
                                //grab the topmost solid block
                                if (GETBLOCK(_blockIDData[index]).meshType == MeshType::BLOCK) {
                                    blocks.push_back(_blockIDData[index]);
                                    break;
                                }
                        }
                    }
                }
                lodIndex = y * dataLayer + z * dataWidth + x;
                if (blocks.empty()) {
                    lodIDData[lodIndex] = 0;
                } else {
                    lodIDData[lodIndex] = blocks[0];
                }
                lodLampData[lodIndex] = 0;
                lodSunData[lodIndex] = 31;
                blocks.clear();
            }
        }
    }

    _blockIDData = lodIDData;
    _lampLightData = lodLampData;
    _sunlightData = lodSunData;
}
