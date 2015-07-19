///
/// NFloraGenerator.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 15 Jul 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Generates flora and trees
///

#pragma once

#ifndef NFloraGenerator_h__
#define NFloraGenerator_h__

#include "Flora.h"
#include "Chunk.h"

// 0111111111 0111111111 0111111111 = 0x1FF7FDFF
#define NO_CHUNK_OFFSET 0x1FF7FDFF

// Will not work for chunks > 32^3
struct FloraNode {
    FloraNode(ui16 blockID, ui16 blockIndex, ui32 chunkOffset) :
        blockID(blockID), blockIndex(blockIndex), chunkOffset(chunkOffset) {
    };
    ui16 blockID;
    ui16 blockIndex;
    // TODO(Ben): ui32 instead for massive trees? Use leftover bits for Y?
    ui32 chunkOffset; ///< Packed 00 XXXXXXXXXX YYYYYYYYYY ZZZZZZZZZZ for positional offset. 00111 == 0
};

class NFloraGenerator {
public:
    /// @brief Generates flora for a chunk using its QueuedFlora.
    /// @param chunk: Chunk who's flora should be generated.
    /// @param gridData: The heightmap to use
    /// @param fNodes: Returned low priority nodes, for flora and leaves.
    /// @param wNodes: Returned high priority nodes, for tree "wood".
    void generateChunkFlora(const Chunk* chunk, const PlanetHeightData* heightData, OUT std::vector<FloraNode>& fNodes, OUT std::vector<FloraNode>& wNodes);
    /// Generates standalone tree.
    void generateTree(const NTreeType* type, f32 age, OUT std::vector<FloraNode>& fNodes, OUT std::vector<FloraNode>& wNodes, ui32 chunkOffset = NO_CHUNK_OFFSET, ui16 blockIndex = 0);
    /// Generates standalone flora.
    void generateFlora(FloraData type, f32 age, OUT std::vector<FloraNode>& fNodes, OUT std::vector<FloraNode>& wNodes, ui32 chunkOffset = NO_CHUNK_OFFSET, ui16 blockIndex = 0);
    /// Generates a specific tree's properties
    static void generateTreeProperties(const NTreeType* type, f32 age, OUT TreeData& tree);

    static inline int getChunkXOffset(ui32 chunkOffset) {
        return (int)((chunkOffset >> 20) & 0x3FF) - 0x1FF;
    }
    static inline int getChunkYOffset(ui32 chunkOffset) {
        return (int)((chunkOffset >> 10) & 0x3FF) - 0x1FF;
    }
    static inline int getChunkZOffset(ui32 chunkOffset) {
        return (int)(chunkOffset & 0x3FF) - 0x1FF;
    }
private:
    enum TreeDir {
        TREE_LEFT = 0, TREE_BACK, TREE_RIGHT, TREE_FRONT, TREE_UP, TREE_DOWN, TREE_NO_DIR
    };

    void makeTrunkSlice(ui32 chunkOffset, const TreeTrunkProperties& props);
    void generateBranch(ui32 chunkOffset, int x, int y, int z, ui32 segments, f32 length, f32 width, const f32v3& dir, const TreeBranchProperties& props);
    void generateLeaves(ui32 chunkOffset, int x, int y, int z, const TreeLeafProperties& props);
    void generateRoundLeaves(ui32 chunkOffset, int x, int y, int z, const TreeLeafProperties& props);
    void generateClusterLeaves(ui32 chunkOffset, int x, int y, int z, const TreeLeafProperties& props);
    void generateYSlice(ui32 chunkOffset, int x, int y, int z, int radius, ui16 blockID, std::vector<FloraNode>& nodes);
    void directionalMove(ui16& blockIndex, ui32 &chunkOffset, TreeDir dir);
   
    std::vector<FloraNode>* m_fNodes;
    std::vector<FloraNode>* m_wNodes;
    TreeData m_treeData;
    FloraData m_floraData;
    ui16 m_centerX, m_centerY, m_centerZ;
};

#endif // NFloraGenerator_h__