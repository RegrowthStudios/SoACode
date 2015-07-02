///
/// ChunkGrid.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 26 Feb 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Grid of chunks for a voxel world
///

#pragma once

#ifndef ChunkGrid_h__
#define ChunkGrid_h__

#include "VoxelCoordinateSpaces.h"
#include "ChunkGenerator.h"
#include "concurrentqueue.h"
#include "Chunk.h"

#include <memory>
#include <map>
#include <vector>
#include <Vorb/utils.h>

class ChunkAllocator;

class ChunkGrid {
public:
    void init(WorldCubeFace face, ChunkAllocator* chunkAllocator, 
              OPT vcore::ThreadPool<WorkerData>* threadPool,
              ui32 generatorsPerRow,
              PlanetGenData* genData);

    void addChunk(Chunk* chunk);

    void removeChunk(Chunk* chunk, int index);

    Chunk* getChunk(const f64v3& voxelPos);

    Chunk* getChunk(const i32v3& chunkPos);

    const Chunk* getChunk(const i32v3& chunkPos) const;

    // Will generate chunk if it doesn't exist
    void submitQuery(ChunkQuery* query);

    /// Gets a chunkGridData for a specific 2D position
    /// @param gridPos: The grid position for the data
    std::shared_ptr<ChunkGridData> getChunkGridData(const i32v2& gridPos) const;

    // Processes chunk queries
    void update();

    const std::vector<Chunk*>& getActiveChunks() const { return m_activeChunks; }

private:
    void connectNeighbors(Chunk* chunk);
    void disconnectNeighbors(Chunk* chunk);

    moodycamel::ConcurrentQueue<ChunkQuery*> m_queries;

    ChunkAllocator* m_allocator = nullptr;
    ChunkGenerator* m_generators = nullptr;

    std::vector<Chunk*> m_activeChunks;

    std::unordered_map<i32v3, Chunk*> m_chunkMap; ///< hashmap of chunks
    std::unordered_map<i32v2, std::shared_ptr<ChunkGridData> > m_chunkGridDataMap; ///< 2D grid specific data
    
    ui32 m_generatorsPerRow;
    ui32 m_numGenerators;
    WorldCubeFace m_face = FACE_NONE;

    static volatile ChunkID m_nextAvailableID;
};

#endif // ChunkGrid_h__
