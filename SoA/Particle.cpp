#include "stdafx.h"
#include "Particle.h"

#include "Chunk.h"
#include "ChunkManager.h"
#include "ParticleEngine.h"
#include "utils.h"

std::vector <ParticleType> particleTypes;

bool Particle::update(const f64v3& batchPosition) {
    i32v3 chPos;
    f32v3 relativePos;
    int bx, by, bz;
    bool fc = 1, bc = 1, lc = 1, rc = 1, tc = 1;
    int c;
    Chunk *ch;

    if(isYCollidable) {
        isYCollidable = 0;
        position.x += velocity.x * physSpeedFactor;
        position.z += velocity.z * physSpeedFactor;
    } else {
        position += velocity * (float)physSpeedFactor;
    }

    relativePos = f64v3(position) - batchPosition;
   
    chPos = GameManager::chunkManager->getChunkPosition(relativePos);

    bx = relativePos.x - chPos.x * CHUNK_WIDTH;
    by = relativePos.y - chPos.y * CHUNK_WIDTH;
    bz = relativePos.z - chPos.z * CHUNK_WIDTH;

    c = bx + by*CHUNK_LAYER + bz*CHUNK_WIDTH;

    if(c != pc) {
        pc = c;


        ch = GameManager::chunkManager->getChunk(chPos);

        if((!ch) || ch->isAccessible == 0) return 0;


        if(ch->getBlock(c).collide) {
            double fx, fy, fz;

            fx = position.x - fastFloor(position.x) - 0.5; //0 means in the center, otherwise we get collision direction
            fy = position.y - fastFloor(position.y) - 0.5;
            fz = position.z - fastFloor(position.z) - 0.5;


            double Afx = ABS(fx);
            double Afy = ABS(fy);
            double Afz = ABS(fz);
            rc = GETBLOCK(ch->getRightBlockData(c)).collide;
            lc = GETBLOCK(ch->getLeftBlockData(c)).collide;
            fc = GETBLOCK(ch->getFrontBlockData(c)).collide;
            bc = GETBLOCK(ch->getBackBlockData(c)).collide;

            if(Afx >= Afy && Afx >= Afz && ((fx > 0 && rc == 0) || (fx <= 0 && (lc == 0)))) { //x collide
                if(fx > 0) {
                    position.x += 0.5001f - Afx;
                } else {
                    position.x -= 0.5001f - Afx;
                }
                if(ABS(velocity.x) < 0.01) {
                    velocity.x = 0;
                } else {
                    velocity.x = velocity.x * -0.8;
                }
            } else if(0 && Afz > Afy && ((fz > 0 && fc == 0) || (fz <= 0 && bc == 0))) { //z collide
                if(fz > 0) {
                    position.z += 0.5001 - Afz;
                } else {
                    position.z -= 0.5001 - Afz;
                }
                if(ABS(velocity.z) < 0.01) {
                    velocity.z = 0;
                } else {
                    velocity.z = velocity.z * -0.8;
                }
            } else { //y collide
                tc = GETBLOCK(ch->getTopBlockData(c)).collide;
                if(fy <= 0 && (Blocks[GETBLOCKTYPE(ch->getBottomBlockData(c))].collide == 0)) {
                    position.y -= 0.5001f - Afy;
                } else if(tc == 0) {
                    position.y += 0.5001f - Afy;
                } else {
                    if(fc == 0) {
                        position.z += 0.5001f - Afz;
                    } else if(bc == 0) {
                        position.z -= 0.5001f - Afz;
                    } else if(lc == 0) {
                        position.x -= 0.5001f - Afx;
                    } else if(rc == 0) {
                        position.x += 0.5001f - Afx;
                    } else {
                        position.x -= velocity.x * physSpeedFactor;
                        position.z -= velocity.z * physSpeedFactor;
                        velocity = glm::dvec3(0.0);
                    }
                }
                if(velocity.y <= 0) {
                    isYCollidable = 1;
                    if(ABS(velocity.y) < GRAVITY) {
                        velocity.y = 0;
                    } else {
                        velocity.y = -(velocity.y*0.6);
                    }
                } else {
                    velocity.y = -(velocity.y*0.6);
                }
                velocity.x *= 0.8;
                velocity.z *= 0.8;
            }

        } else {
            light[0] = (GLubyte)((pow(LIGHT_MULT, MAXLIGHT - (int)ch->getLampLight(c)) - 0.18f) * 255);
            light[1] = (GLubyte)((pow(LIGHT_MULT, MAXLIGHT - (int)ch->getSunlight(c)) - 0.18f) * 255);
        }

    }
    if(!isYCollidable) {
        velocity.y -= GRAVITY * physSpeedFactor;
    }


    return 0;
}

bool Particle::update() {

    switch(type) {
    case PARTICLE_LINEAR:
        position += velocity * (float)physSpeedFactor;
        break;
    }
    return 0;
}
