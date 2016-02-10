#pragma once

#ifndef GameRenderParams_h__
#define GameRenderParams_h__

#include <Vorb/VorbPreDecl.inl>
#include "VoxelSpaceConversions.h"

class ChunkMesh;
class ChunkMeshManager;
class BlockPack;
class BlockTexturePack;
DECL_VG(template <class T>
        class Camera3D)

class GameRenderParams {
public:
    void calculateParams(const f64v3& worldCameraPos,
                         const vg::Camera3D<f64>* ChunkCamera,
                         const VoxelPosition3D& voxPosition,
                         f64 voxelWorldRadius,
                         ChunkMeshManager* ChunkMeshmanager,
                         BlockPack* blocks,
                         BlockTexturePack* blockTexturePack,
                         bool IsUnderwater);

    f32v3 sunlightDirection;
    f32v3 sunlightColor;  
    float sunlightIntensity;
    f32v3 fogColor;
    float fogEnd;
    float fogStart;
    float lightActive;
    const vg::Camera3D<f64>* chunkCamera;
    ChunkMeshManager* chunkMeshmanager;
    BlockPack* blocks;
    BlockTexturePack* blockTexturePack;
    bool isUnderwater;
private:
    void calculateFog(float theta, bool isUnderwater);
    void calculateSunlight(float theta);
};

#endif // GameRenderParams_h__