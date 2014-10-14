#include "stdafx.h"

#include "BlockData.h"
#include "CAEngine.h"
#include "ChunkUpdater.h"
#include "Chunk.h"
#include "Errors.h"
#include "GameManager.h"
#include "ParticleEngine.h"
#include "PhysicsEngine.h"
#include "VoxelLightEngine.h"
#include "VoxelUtils.h"

void ChunkUpdater::randomBlockUpdates(Chunk* chunk)
{
    if (!chunk->isAccessible) return;
    int blockIndex, blockIndex2, blockID;
    ChunkStates newState;
    bool needsSetup;
    Chunk *owner;

    Chunk* left = chunk->left;
    Chunk* right = chunk->right;
    Chunk* front = chunk->front;
    Chunk* back = chunk->back;
    Chunk* bottom = chunk->bottom;
    Chunk* top = chunk->top;

    i32v3 pos;

    for (int i = 0; i < 15; i++){
        needsSetup = false;

        blockIndex = RandomUpdateOrder[chunk->blockUpdateIndex++];
        if (chunk->blockUpdateIndex == CHUNK_SIZE) chunk->blockUpdateIndex = 0;

        pos = getPosFromBlockIndex(blockIndex);
        
        blockID = chunk->getBlockID(blockIndex);

        if (Blocks[blockID].emitterRandom){
            // uncomment for falling leaves
            //    particleEngine.AddAnimatedParticles(1, glm::dvec3(position.x + x, Y + y, position.z + z), Blocks[b].emitterRandom);
        }

        //TODO: Replace most of this with block update scripts
        if (blockID >= LOWWATER && blockID < LOWWATER + 5 && (GETBLOCKTYPE(chunk->getBottomBlockData(blockIndex, pos.y, &blockIndex2, &owner)) < LOWWATER)){
            chunk->setBlockID(blockIndex, NONE);
            owner->numBlocks--;
            needsSetup = true;
            newState = ChunkStates::WATERMESH;
            ChunkUpdater::addBlockToUpdateList(chunk, blockIndex);
        } else if (blockID == FIRE) {
            updateFireBlock(chunk, blockIndex);
            needsSetup = true;
            newState = ChunkStates::MESH;
        } else if (blockID == DIRTGRASS){
            int bt = GETBLOCKTYPE(chunk->getTopBlockData(blockIndex, pos.y, &blockIndex2, &owner));
            if ((Blocks[bt].collide && bt != LEAVES1) || bt >= LOWWATER){
                chunk->setBlockID(blockIndex, DIRT);
                needsSetup = true;
                newState = ChunkStates::MESH;
            }
        } else if (blockID == DIRT){
            if ((rand() % 10 == 0) && (GETBLOCKTYPE(chunk->getTopBlockData(blockIndex, pos.y, &blockIndex2, &owner)) == NONE) && (GETBLOCKTYPE(chunk->getLeftBlockData(blockIndex, pos.x, &blockIndex2, &owner)) == DIRTGRASS || GETBLOCKTYPE(chunk->getRightBlockData(blockIndex, pos.x, &blockIndex2, &owner)) == DIRTGRASS ||
                GETBLOCKTYPE(chunk->getFrontBlockData(blockIndex, pos.z, &blockIndex2, &owner)) == DIRTGRASS || GETBLOCKTYPE(chunk->getBackBlockData(blockIndex, pos.z, &blockIndex2, &owner)) == DIRTGRASS)){
                chunk->setBlockID(blockIndex, DIRTGRASS);
                needsSetup = true;
                newState = ChunkStates::MESH;
            }
        }

        if (needsSetup){
            
            chunk->changeState(newState);
            if (pos.x == 0){
                if (left && left->isAccessible){
                    left->changeState(newState);
                }
            } else if (pos.x == CHUNK_WIDTH - 1){
                if (right && right->isAccessible){
                    right->changeState(newState);
                }
            }
            if (pos.y == 0){
                if (bottom && bottom->isAccessible){
                    bottom->changeState(newState);
                }
            } else if (pos.y == CHUNK_WIDTH - 1){
                if (top && top->isAccessible){
                    top->changeState(newState);
                }
            }
            if (pos.z == 0){
                if (back && back->isAccessible){
                    back->changeState(newState);
                }
            } else if (pos.z == CHUNK_WIDTH - 1){
                if (front && front->isAccessible){
                    front->changeState(newState);
                }
            }
        }
    }
}

void ChunkUpdater::placeBlock(Chunk* chunk, int blockIndex, int blockType)
{
    //If you call placeBlock with the air block, call remove block
    if (blockType == NONE) {
        removeBlock(chunk, blockIndex, false);
        return;
    }

    Block &block = GETBLOCK(blockType);

    if (chunk->getBlockData(blockIndex) == NONE) {
        chunk->numBlocks++;
    }
    chunk->setBlockData(blockIndex, blockType);

    if (block.spawnerVal || block.sinkVal){
        chunk->spawnerBlocks.push_back(blockIndex);
    }

    const i32v3 pos = getPosFromBlockIndex(blockIndex);

    if (block.emitter){
        particleEngine.addEmitter(block.emitter, glm::dvec3(chunk->gridPosition.x + pos.x, chunk->gridPosition.y + pos.y, chunk->gridPosition.z + pos.z), blockType);
    }

    //Check for light removal due to block occlusion
    if (block.blockLight) {

        if (chunk->getSunlight(blockIndex)){
            if (chunk->getSunlight(blockIndex) == MAXLIGHT){
                chunk->setSunlight(blockIndex, 0);
                chunk->sunRemovalList.push_back(blockIndex);
            } else {
                chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
                chunk->setSunlight(blockIndex, 0);
            }
        }

        if (chunk->getLampLight(blockIndex)){
            chunk->lampLightRemovalQueue.push_back(LampLightRemovalNode(blockIndex, chunk->getLampLight(blockIndex)));
            chunk->setLampLight(blockIndex, 0);
        }
    } else if (block.colorFilter != f32v3(1.0f)) {
        //This will pull light from neighbors
        chunk->lampLightRemovalQueue.push_back(LampLightRemovalNode(blockIndex, chunk->getLampLight(blockIndex)));
        chunk->setLampLight(blockIndex, 0);
    }
    //Light placement
    if (block.isLight){
        chunk->setLampLight(blockIndex, block.lightColor);
        chunk->lampLightUpdateQueue.push_back(LampLightUpdateNode(blockIndex, block.lightColor));
    }

    ChunkUpdater::addBlockToUpdateList(chunk, blockIndex);
    if (GETBLOCKTYPE(blockType) >= LOWWATER) {
        chunk->changeState(ChunkStates::WATERMESH);
        updateNeighborStates(chunk, pos, ChunkStates::WATERMESH);
    } else{
        chunk->changeState(ChunkStates::MESH);
        updateNeighborStates(chunk, pos, ChunkStates::MESH);

    }
    chunk->dirty = true;
}

//Simplified placeBlock for liquid physics
void ChunkUpdater::placeBlockFromLiquidPhysics(Chunk* chunk, int blockIndex, int blockType)
{
    Block &block = GETBLOCK(blockType);

    if (chunk->getBlockData(blockIndex) == NONE) {
        chunk->numBlocks++;
    }
    chunk->setBlockData(blockIndex, blockType);

    //Check for light removal due to block occlusion
    if (block.blockLight) {

        if (chunk->getSunlight(blockIndex)){
            if (chunk->getSunlight(blockIndex) == MAXLIGHT){
                chunk->setSunlight(blockIndex, 0);
                chunk->sunRemovalList.push_back(blockIndex);
            } else {
                chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
                chunk->setSunlight(blockIndex, 0);
            }
        }

        if (chunk->getLampLight(blockIndex)){
            chunk->lampLightRemovalQueue.push_back(LampLightRemovalNode(blockIndex, chunk->getLampLight(blockIndex)));
            chunk->setLampLight(blockIndex, 0);
        }
    }

    //Light placement
    if (block.isLight){
        chunk->setLampLight(blockIndex, block.lightColor);
        chunk->lampLightUpdateQueue.push_back(LampLightUpdateNode(blockIndex, block.lightColor));
    }

    ChunkUpdater::addBlockToUpdateList(chunk, blockIndex);
  
    chunk->dirty = true;
}

void ChunkUpdater::removeBlock(Chunk* chunk, int blockIndex, bool isBreak, double force, glm::vec3 explodeDir)
{
    int blockID = chunk->getBlockID(blockIndex);
    const Block &block = Blocks[blockID];

    float explodeDist = glm::length(explodeDir);

    const i32v3 pos = getPosFromBlockIndex(blockIndex);

    if (chunk->getBlockID(blockIndex) == 0) {
        cout << "ALREADY REMOVED\n";
    }

    GLbyte da, db, dc;

    //Falling check
    if (explodeDist){
        GLubyte d;
        da = (GLbyte)(explodeDir.x);
        db = (GLbyte)(explodeDir.y);
        dc = (GLbyte)(explodeDir.z);
        if (explodeDist > 255){
            d = 255;
        } else{
            d = (GLubyte)(explodeDist);
        }
        GameManager::physicsEngine->addFallingCheckNode(FallingCheckNode(chunk, blockIndex, da, db, dc, d));
    } else{
        GameManager::physicsEngine->addFallingCheckNode(FallingCheckNode(chunk, blockIndex));
    }

    //If we are braking the block rather than removing it, it should explode or emit particles
    if (isBreak) {
        if (block.explosivePower){
            glm::dvec3 dtmp(chunk->gridPosition.x + pos.x, chunk->gridPosition.y + pos.y, chunk->gridPosition.z + pos.z);
            GameManager::physicsEngine->addExplosion(ExplosionNode(dtmp, blockID));
        }

        if (block.emitterOnBreak && block.explosivePower == 0){ // 
            particleEngine.addEmitter(block.emitterOnBreak, glm::dvec3(chunk->gridPosition.x + blockIndex%CHUNK_WIDTH, chunk->gridPosition.y + blockIndex / CHUNK_LAYER, chunk->gridPosition.z + (blockIndex%CHUNK_LAYER) / CHUNK_WIDTH), blockID);
        }
        if (explodeDist){
            float expForce = glm::length(explodeDir);
            expForce = pow(0.89, expForce)*0.6;
            if (expForce < 0.0f) expForce = 0.0f;
            breakBlock(chunk, pos.x, pos.y, pos.z, blockID, force, -glm::normalize(explodeDir)*expForce);
        } else{
            breakBlock(chunk, pos.x, pos.y, pos.z, blockID, force);
        }
    }
    chunk->setBlockData(blockIndex, NONE);

    //Update lighting
    if (block.blockLight || block.isLight){
        //This will pull light from neighbors
        chunk->lampLightRemovalQueue.push_back(LampLightRemovalNode(blockIndex, chunk->getLampLight(blockIndex)));
        chunk->setLampLight(blockIndex, 0);

        //sunlight update
        if (pos.y < CHUNK_WIDTH - 1) {
            if (chunk->getSunlight(blockIndex + CHUNK_LAYER) == MAXLIGHT) {
                chunk->setSunlight(blockIndex, MAXLIGHT);
                chunk->sunExtendList.push_back(blockIndex);
            } else {
                //This will pull light from neighbors
                chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
                chunk->setSunlight(blockIndex, 0);
            }
        } else if (chunk->top && chunk->top->isAccessible) {
            if (chunk->top->getSunlight(blockIndex + CHUNK_LAYER - CHUNK_SIZE) == MAXLIGHT) {
                chunk->setSunlight(blockIndex, MAXLIGHT);
                chunk->sunExtendList.push_back(blockIndex);
            } else {
                //This will pull light from neighbors
                chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
                chunk->setSunlight(blockIndex, 0);
            }
        } else {
            //This will pull light from neighbors
            chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
            chunk->setSunlight(blockIndex, 0);
        }
    }

    ChunkUpdater::addBlockToUpdateList(chunk, blockIndex);
    chunk->numBlocks--;
    if (chunk->numBlocks < 0) chunk->numBlocks = 0;


    chunk->changeState(ChunkStates::MESH);
    chunk->dirty = 1;

    updateNeighborStates(chunk, pos, ChunkStates::MESH);
}

void ChunkUpdater::removeBlockFromLiquidPhysics(Chunk* chunk, int blockIndex)
{
    const Block &block = chunk->getBlock(blockIndex);

    const i32v3 pos = getPosFromBlockIndex(blockIndex);
  
    chunk->setBlockData(blockIndex, NONE);

    //Update lighting
    if (block.blockLight || block.isLight){
        //This will pull light from neighbors
        chunk->lampLightRemovalQueue.push_back(LampLightRemovalNode(blockIndex, chunk->getLampLight(blockIndex)));
        chunk->setLampLight(blockIndex, 0);

        //sunlight update
        if (pos.y < CHUNK_WIDTH - 1) {
            if (chunk->getSunlight(blockIndex + CHUNK_LAYER) == MAXLIGHT) {
                chunk->setSunlight(blockIndex, MAXLIGHT);
                chunk->sunExtendList.push_back(blockIndex);
            } else {
                //This will pull light from neighbors
                chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
                chunk->setSunlight(blockIndex, 0);
            }
        } else if (chunk->top && chunk->top->isAccessible) {
            if (chunk->top->getSunlight(blockIndex + CHUNK_LAYER - CHUNK_SIZE) == MAXLIGHT) {
                chunk->setSunlight(blockIndex, MAXLIGHT);
                chunk->sunExtendList.push_back(blockIndex);
            } else {
                //This will pull light from neighbors
                chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
                chunk->setSunlight(blockIndex, 0);
            }
        } else {
            //This will pull light from neighbors
            chunk->sunlightRemovalQueue.push_back(SunlightRemovalNode(blockIndex, chunk->getSunlight(blockIndex)));
            chunk->setSunlight(blockIndex, 0);
        }
    }

    ChunkUpdater::addBlockToUpdateList(chunk, blockIndex);
    chunk->numBlocks--;
    if (chunk->numBlocks < 0) chunk->numBlocks = 0;


    chunk->changeState(ChunkStates::WATERMESH);
    chunk->dirty = 1;
}

void ChunkUpdater::updateNeighborStates(Chunk* chunk, const i32v3& pos, ChunkStates state) {
    if (pos.x == 0){
        if (chunk->left){
            chunk->left->changeState(state);
        }
    } else if (pos.x == CHUNK_WIDTH - 1){
        if (chunk->right){
            chunk->right->changeState(state);
        }
    }
    if (pos.y == 0){
        if (chunk->bottom){
            chunk->bottom->changeState(state);
        }
    } else if (pos.y == CHUNK_WIDTH - 1){
        if (chunk->top){
            chunk->top->changeState(state);
        }
    }
    if (pos.z == 0){
        if (chunk->back){
            chunk->back->changeState(state);
        }
    } else if (pos.z == CHUNK_WIDTH - 1){
        if (chunk->front){
            chunk->front->changeState(state);
        }
    }
}

void ChunkUpdater::updateNeighborStates(Chunk* chunk, int blockIndex, ChunkStates state) {
    const i32v3 pos = getPosFromBlockIndex(blockIndex);
    if (pos.x == 0){
        if (chunk->left){
            chunk->left->changeState(state);
        }
    } else if (pos.x == CHUNK_WIDTH - 1){
        if (chunk->right){
            chunk->right->changeState(state);
        }
    }
    if (pos.y == 0){
        if (chunk->bottom){
            chunk->bottom->changeState(state);
        }
    } else if (pos.y == CHUNK_WIDTH - 1){
        if (chunk->top){
            chunk->top->changeState(state);
        }
    }
    if (pos.z == 0){
        if (chunk->back){
            chunk->back->changeState(state);
        }
    } else if (pos.z == CHUNK_WIDTH - 1){
        if (chunk->front){
            chunk->front->changeState(state);
        }
    }
}

void ChunkUpdater::addBlockToUpdateList(Chunk* chunk, int c)
{
    int phys;
    const i32v3 pos = getPosFromBlockIndex(c);

    Chunk* left = chunk->left;
    Chunk* right = chunk->right;
    Chunk* front = chunk->front;
    Chunk* back = chunk->back;
    Chunk* top = chunk->top;
    Chunk* bottom = chunk->bottom;

    if ((phys = chunk->getBlock(c).physicsProperty - physStart) > -1) {
        chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c);
    }

    if (pos.x > 0){ //left
        if ((phys = chunk->getBlock(c - 1).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c - 1);
        }
    } else if (left && left->isAccessible){
        if ((phys = left->getBlock(c + CHUNK_WIDTH - 1).physicsProperty - physStart) > -1) {
            left->blockUpdateList[phys][left->activeUpdateList[phys]].push_back(c + CHUNK_WIDTH - 1);
        }
    } else{
        return;
    }

    if (pos.x < CHUNK_WIDTH - 1){ //right
        if ((phys = chunk->getBlock(c + 1).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c + 1);
        }
    } else if (right && right->isAccessible){
        if ((phys = right->getBlock(c - CHUNK_WIDTH + 1).physicsProperty - physStart) > -1) {
            right->blockUpdateList[phys][right->activeUpdateList[phys]].push_back(c - CHUNK_WIDTH + 1);
        }
    } else{
        return;
    }

    if (pos.z > 0){ //back
        if ((phys = chunk->getBlock(c - CHUNK_WIDTH).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c - CHUNK_WIDTH);
        }
    } else if (back && back->isAccessible){
        if ((phys = back->getBlock(c + CHUNK_LAYER - CHUNK_WIDTH).physicsProperty - physStart) > -1) {
            back->blockUpdateList[phys][back->activeUpdateList[phys]].push_back(c + CHUNK_LAYER - CHUNK_WIDTH);
        }
    } else{
        return;
    }

    if (pos.z < CHUNK_WIDTH - 1){ //front
        if ((phys = chunk->getBlock(c + CHUNK_WIDTH).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c + CHUNK_WIDTH);
        }
    } else if (front && front->isAccessible){
        if ((phys = front->getBlock(c - CHUNK_LAYER + CHUNK_WIDTH).physicsProperty - physStart) > -1) {
            front->blockUpdateList[phys][front->activeUpdateList[phys]].push_back(c - CHUNK_LAYER + CHUNK_WIDTH);
        }
    } else{
        return;
    }

    if (pos.y > 0){ //bottom
        if ((phys = chunk->getBlock(c - CHUNK_LAYER).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c - CHUNK_LAYER);
        }
    } else if (bottom && bottom->isAccessible){
        if ((phys = bottom->getBlock(CHUNK_SIZE - CHUNK_LAYER + c).physicsProperty - physStart) > -1) {
            bottom->blockUpdateList[phys][bottom->activeUpdateList[phys]].push_back(CHUNK_SIZE - CHUNK_LAYER + c);
        }
    } else{
        return;
    }

    if (pos.y < CHUNK_WIDTH - 1){ //top
        if ((phys = chunk->getBlock(c + CHUNK_LAYER).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c + CHUNK_LAYER);
        }
    } else if (top && top->isAccessible){
        if ((phys = top->getBlock(c - CHUNK_SIZE + CHUNK_LAYER).physicsProperty - physStart) > -1) {
            top->blockUpdateList[phys][top->activeUpdateList[phys]].push_back(c - CHUNK_SIZE + CHUNK_LAYER);
        }
    }
}

void ChunkUpdater::snowAddBlockToUpdateList(Chunk* chunk, int c)
{
    int phys;
    const i32v3 pos = getPosFromBlockIndex(c);

    if ((phys = chunk->getBlock(c).physicsProperty - physStart) > -1) {
        chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c);
    }

    if (pos.y > 0){ //bottom
        if ((phys = chunk->getBlock(c - CHUNK_LAYER).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c - CHUNK_LAYER);
        }
    } else if (chunk->bottom && chunk->bottom->isAccessible){
        if ((phys = chunk->bottom->getBlock(CHUNK_SIZE - CHUNK_LAYER + c).physicsProperty - physStart) > -1) {
            chunk->bottom->blockUpdateList[phys][chunk->bottom->activeUpdateList[phys]].push_back(CHUNK_SIZE - CHUNK_LAYER + c);
        }
    } else{
        return;
    }

    if (pos.y < CHUNK_WIDTH - 1){ //top
        if ((phys = chunk->getBlock(c + CHUNK_LAYER).physicsProperty - physStart) > -1) {
            chunk->blockUpdateList[phys][chunk->activeUpdateList[phys]].push_back(c + CHUNK_LAYER);
        }
    } else if (chunk->top && chunk->top->isAccessible){
        if ((phys = chunk->top->getBlock(c - CHUNK_SIZE + CHUNK_LAYER).physicsProperty - physStart) > -1) {
            chunk->top->blockUpdateList[phys][chunk->top->activeUpdateList[phys]].push_back(c - CHUNK_SIZE + CHUNK_LAYER);
        }
    }
}

//TODO: Replace this with simple emitterOnBreak
//This function name is misleading, ignore for now
void ChunkUpdater::breakBlock(Chunk* chunk, int x, int y, int z, int blockType, double force, glm::vec3 extraForce)
{
    glm::vec4 color;
    int btype = GETBLOCKTYPE(blockType);
    GLuint flags = GETFLAGS(blockType);

    color.a = 255;

    if (Blocks[btype].altColors.size() >= flags && flags){
        color.r = Blocks[btype].altColors[flags - 1].r;
        color.g = Blocks[btype].altColors[flags - 1].g;
        color.b = Blocks[btype].altColors[flags - 1].b;
        //    cout << btype << " " << flags-1 << " ";
    } else{
        color.r = Blocks[btype].color[0];
        color.g = Blocks[btype].color[1];
        color.b = Blocks[btype].color[2];
    }

    if (Blocks[btype].meshType != MeshType::NONE && Blocks[btype].explosivePower == 0){
        if (!chunk->mesh || chunk->mesh->inFrustum){
            particleEngine.addParticles(BPARTICLES, glm::dvec3(chunk->gridPosition.x + x, chunk->gridPosition.y + y, chunk->gridPosition.z + z), 0, 0.1, 0, 1, color, Blocks[btype].pxTex, 2.0f, 4, extraForce);
        }
    }
}

float ChunkUpdater::getBurnProbability(Chunk* chunk, int blockIndex)
{

    float flammability = 0.0f;
    //bottom
    int bt = chunk->getBottomBlockData(blockIndex);
    flammability += GETBLOCK(bt).flammability;
    bt = chunk->getLeftBlockData(blockIndex);
    flammability += GETBLOCK(bt).flammability;
    //right
    bt = chunk->getRightBlockData(blockIndex);
    flammability += GETBLOCK(bt).flammability;
    //back
    bt = chunk->getBackBlockData(blockIndex);
    flammability += GETBLOCK(bt).flammability;
    //front
    bt = chunk->getFrontBlockData(blockIndex);
    flammability += GETBLOCK(bt).flammability;
    //top
    bt = chunk->getTopBlockData(blockIndex);
    flammability += GETBLOCK(bt).flammability;

    if (flammability < 0) return 0.0f;

    return flammability / 6.0f;
}

void ChunkUpdater::updateFireBlock(Chunk* chunk, int blockIndex){
    //left
    int blockIndex2, blockIndex3, blockIndex4;
    Chunk *owner2, *owner3, *owner4;
    int bt;

    const i32v3 pos = getPosFromBlockIndex(blockIndex);

    const float sideTopMult = 1.5;
    const float topMult = 2.0;
    const float sideBotMult = 0.5;
    const float botMult = 0.8;

    burnAdjacentBlocks(chunk, blockIndex);

    //********************************************************left
    bt = chunk->getLeftBlockData(blockIndex, pos.x, &blockIndex2, &owner2);

    checkBurnBlock(blockIndex2, bt, owner2);

    //left front
    bt = owner2->getFrontBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3);

    //left back
    bt = owner2->getBackBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3);

    //left top
    bt = owner2->getTopBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideTopMult);


    //left top front
    bt = owner3->getFrontBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideTopMult);

    //left top back
    bt = owner3->getBackBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideTopMult);

    //left bottom
    bt = owner2->getBottomBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideBotMult);

    //left bottom front
    bt = owner3->getFrontBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideBotMult);

    //left bottom back
    bt = owner3->getBackBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideBotMult);

    //********************************************************right
    bt = chunk->getRightBlockData(blockIndex, pos.x, &blockIndex2, &owner2);

    checkBurnBlock(blockIndex2, bt, owner2);

    //left front
    bt = owner2->getFrontBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3);

    //left back
    bt = owner2->getBackBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3);

    //left top
    bt = owner2->getTopBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideTopMult);


    //left top front
    bt = owner3->getFrontBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideTopMult);

    //left top back
    bt = owner3->getBackBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideTopMult);

    //left bottom
    bt = owner2->getBottomBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideBotMult);

    //left bottom front
    bt = owner3->getFrontBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideBotMult);

    //left bottom back
    bt = owner3->getBackBlockData(blockIndex3, (blockIndex3 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex4, &owner4);
    checkBurnBlock(blockIndex4, bt, owner4, sideBotMult);

    //******************************************************front
    bt = chunk->getFrontBlockData(blockIndex, pos.z, &blockIndex2, &owner2);

    checkBurnBlock(blockIndex2, bt, owner2);

    //front top
    bt = owner2->getTopBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideTopMult);

    //front bottom
    bt = owner2->getBottomBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideBotMult);

    //********************************************************back
    bt = chunk->getBackBlockData(blockIndex, pos.z, &blockIndex2, &owner2);

    checkBurnBlock(blockIndex2, bt, owner2);

    //back top
    bt = owner2->getTopBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideTopMult);

    //back bottom
    bt = owner2->getBottomBlockData(blockIndex2, blockIndex2 / CHUNK_LAYER, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideBotMult);

    //********************************************************top
    bt = chunk->getTopBlockData(blockIndex, pos.y, &blockIndex2, &owner2);
    checkBurnBlock(blockIndex2, bt, owner2, topMult);

    //top front
    bt = owner2->getFrontBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideTopMult);

    //top back
    bt = owner2->getBackBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideTopMult);


    //********************************************************bottom
    bt = chunk->getBottomBlockData(blockIndex, pos.y, &blockIndex2, &owner2);
    checkBurnBlock(blockIndex2, bt, owner2, botMult);

    //bottom front
    bt = owner2->getFrontBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideBotMult);

    //bottom back
    bt = owner2->getBackBlockData(blockIndex2, (blockIndex2 % CHUNK_LAYER) / CHUNK_WIDTH, &blockIndex3, &owner3);
    checkBurnBlock(blockIndex3, bt, owner3, sideBotMult);

    removeBlock(chunk, blockIndex, false);
}

void ChunkUpdater::burnAdjacentBlocks(Chunk* chunk, int blockIndex){

    int blockIndex2;
    Chunk *owner2;
    Block *b;

    const i32v3 pos = getPosFromBlockIndex(blockIndex);

    int bt = chunk->getBottomBlockData(blockIndex, pos.y, &blockIndex2, &owner2);
    b = &(GETBLOCK(bt));
    if (b->flammability){
        if (b->burnTransformID == NONE){
            removeBlock(owner2, blockIndex2, true);
        } else{
            if (Blocks[b->burnTransformID].emitter){
                particleEngine.addEmitter(Blocks[b->burnTransformID].emitter, glm::dvec3(owner2->gridPosition.x + blockIndex2%CHUNK_WIDTH, owner2->gridPosition.y + blockIndex2 / CHUNK_LAYER, owner2->gridPosition.z + (blockIndex2%CHUNK_LAYER) / CHUNK_WIDTH), b->burnTransformID);
            }
            owner2->setBlockData(blockIndex2, b->burnTransformID);
        }
        owner2->changeState(ChunkStates::MESH);
    }
    //left
    bt = chunk->getLeftBlockData(blockIndex, pos.x, &blockIndex2, &owner2);
    b = &(GETBLOCK(bt));
    if (b->flammability){
        if (b->burnTransformID == NONE){
            removeBlock(owner2, blockIndex2, true);
        } else{
            if (Blocks[b->burnTransformID].emitter){
                particleEngine.addEmitter(Blocks[b->burnTransformID].emitter, glm::dvec3(owner2->gridPosition.x + blockIndex2%CHUNK_WIDTH, owner2->gridPosition.y + blockIndex2 / CHUNK_LAYER, owner2->gridPosition.z + (blockIndex2%CHUNK_LAYER) / CHUNK_WIDTH), b->burnTransformID);
            }
            owner2->setBlockData(blockIndex2, b->burnTransformID);
        }
        owner2->changeState(ChunkStates::MESH);
    }
    //right
    bt = chunk->getRightBlockData(blockIndex, pos.x, &blockIndex2, &owner2);
    b = &(GETBLOCK(bt));
    if (b->flammability){
        if (b->burnTransformID == NONE){
            removeBlock(owner2, blockIndex2, true);
        } else{
            if (Blocks[b->burnTransformID].emitter){
                particleEngine.addEmitter(Blocks[b->burnTransformID].emitter, glm::dvec3(owner2->gridPosition.x + blockIndex2%CHUNK_WIDTH, owner2->gridPosition.y + blockIndex2 / CHUNK_LAYER, owner2->gridPosition.z + (blockIndex2%CHUNK_LAYER) / CHUNK_WIDTH), b->burnTransformID);
            }
            owner2->setBlockData(blockIndex2, b->burnTransformID);
        }
        owner2->changeState(ChunkStates::MESH);
    }
    //back
    bt = chunk->getBackBlockData(blockIndex, pos.z, &blockIndex2, &owner2);
    b = &(GETBLOCK(bt));
    if (b->flammability){
        if (b->burnTransformID == NONE){
            removeBlock(owner2, blockIndex2, true);
        } else{
            if (Blocks[b->burnTransformID].emitter){
                particleEngine.addEmitter(Blocks[b->burnTransformID].emitter, glm::dvec3(owner2->gridPosition.x + blockIndex2%CHUNK_WIDTH, owner2->gridPosition.y + blockIndex2 / CHUNK_LAYER, owner2->gridPosition.z + (blockIndex2%CHUNK_LAYER) / CHUNK_WIDTH), b->burnTransformID);
            }
            owner2->setBlockData(blockIndex2, b->burnTransformID);
        }
        owner2->changeState(ChunkStates::MESH);
    }
    //front
    bt = chunk->getFrontBlockData(blockIndex, pos.z, &blockIndex2, &owner2);
    b = &(GETBLOCK(bt));
    if (b->flammability){
        if (b->burnTransformID == NONE){
            removeBlock(owner2, blockIndex2, true);
        } else{
            if (Blocks[b->burnTransformID].emitter){
                particleEngine.addEmitter(Blocks[b->burnTransformID].emitter, glm::dvec3(owner2->gridPosition.x + blockIndex2%CHUNK_WIDTH, owner2->gridPosition.y + blockIndex2 / CHUNK_LAYER, owner2->gridPosition.z + (blockIndex2%CHUNK_LAYER) / CHUNK_WIDTH), b->burnTransformID);
            }
            owner2->setBlockData(blockIndex2, b->burnTransformID);
        }
        owner2->changeState(ChunkStates::MESH);
    }
    //top
    bt = chunk->getTopBlockData(blockIndex, pos.y, &blockIndex2, &owner2);
    b = &(GETBLOCK(bt));
    if (b->flammability){
        if (b->burnTransformID == NONE){
            removeBlock(owner2, blockIndex2, true);
        } else{
            if (Blocks[b->burnTransformID].emitter){
                particleEngine.addEmitter(Blocks[b->burnTransformID].emitter, glm::dvec3(owner2->gridPosition.x + blockIndex2%CHUNK_WIDTH, owner2->gridPosition.y + blockIndex2 / CHUNK_LAYER, owner2->gridPosition.z + (blockIndex2%CHUNK_LAYER) / CHUNK_WIDTH), b->burnTransformID);
            }
            owner2->setBlockData(blockIndex2, b->burnTransformID);
        }
        owner2->changeState(ChunkStates::MESH);
    }
}

void ChunkUpdater::checkBurnBlock(int blockIndex, int blockType, Chunk *owner, float burnMult)
{
    float burnProb;
    if ((blockType == NONE || GETBLOCK(blockType).waterBreak)){
        burnProb = getBurnProbability(owner, blockIndex) * burnMult;
        if (burnProb > 0){
            float r = rand() / (float)RAND_MAX;
            if (r <= burnProb){
               placeBlock(owner, blockIndex, FIRE);
            }
        }
    }
}