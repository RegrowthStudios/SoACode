//
// ROAMTerrainRenderer.h
// Seed of Andromeda
//
// Created by Benjamin Arnold on 4 Jun 2016
// Copyright 2014 Regrowth Studios
// All Rights Reserved
//
// Summary:
// Renders planetary terrain using the ROAM method.
// http://www.gamasutra.com/view/feature/131596/realtime_dynamic_level_of_detail_.php
//

// This is currently in the prototype stage

#pragma once

#ifndef ROAMTerrainRenderer_h__
#define ROAMTerrainRenderer_h__

#include <Vorb/graphics/gtypes.h>
#include "VoxelCoordinateSpaces.h"

// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (9)

// TODO(Ben): Vary this
// Diameter of a planet cube in patches
#define ROAM_PATCH_DIAMETER (10)
#define ROAM_PATCH_RADIUS (5)

// How many TriTreeNodes should be allocated?
#define POOL_SIZE (25000)

/// Vertex for terrain patch
class ROAMTerrainVertex {
public:
    f32v3     position;    
    f32v3     normal;      
    ColorRGB8 color;       
    ui8       padding;     
    ui8       temperature; 
    ui8       humidity;    
    ui8       padding2[2];
};
static_assert(sizeof(ROAMTerrainVertex) == 32, "ROAMTerrainVertex is not 32");

class TmpROAMTerrainVertex {
public:
    f32v3     position;
};

// Base is hypotenuse
//        ^
//       /|\
//   LN / | \ RN
//     /LC|RC\
//    /___|___\
//        BN
struct ROAMTriTreeNode {
    ROAMTriTreeNode * leftChild     = nullptr; // LC
    ROAMTriTreeNode * rightChild    = nullptr; // RC
    ROAMTriTreeNode * baseNeighbor  = nullptr; // BN
    ROAMTriTreeNode * leftNeighbor  = nullptr; // LN
    ROAMTriTreeNode * rightNeighbor = nullptr; // RN
};

class ROAMPatchRecycler {
public:
    static ROAMTriTreeNode * AllocateTri() {
        vorb_assert(m_nextTriNode < POOL_SIZE, "TMP Bounds");

        return &m_triPool[m_nextTriNode++];
    }

private:
    static int	           m_nextTriNode;               // Index to next free TriTreeNode
    static ROAMTriTreeNode m_triPool[POOL_SIZE]; // Pool of TriTree nodes for splitting

};

class ROAMPatch {
public:
    void init(const f64v2& gridPosition, WorldCubeFace cubeFace, f64 width);
    void reset();

    void tesselate();

    void render();

private:
    void recursTessellate(ROAMTriTreeNode * tri,
                          f64v2 left,
                          f64v2 right,
                          f64v2 apex,
                          int node);

    void split(ROAMTriTreeNode * tri);

    void generateMesh();
    void recursGenerateMesh(ROAMTriTreeNode *tri, f64v2 leftX, f64v2 right, f64v2 apex);


    f64v2 m_gridPos  = f64v2(0.0); ///< Position on 2D grid
    f64v3 m_aabbPos  = f64v3(0.0); ///< Position relative to world
    f64v3 m_aabbDims = f64v3(0.0);
    f64   m_width;

    ui8   m_varianceLeft [1 << (VARIANCE_DEPTH)]; // Left variance tree
    ui8   m_varianceRight[1 << (VARIANCE_DEPTH)]; // Right variance tree

    ui8 * m_currentVariance; // Which variance we are currently using. [Only valid during the Tessellate and ComputeVariance passes]
    ui8   m_varianceDirty;   // Does the Variance Tree need to be recalculated for this Patch?
    ui8   m_isVisible;       // Is this patch visible in the current frame?
    bool  m_isDirty = true;

    ROAMTriTreeNode m_baseLeft;  // Left base triangle tree node
    ROAMTriTreeNode m_baseRight; // Right base triangle tree node

    WorldCubeFace m_cubeFace;  // Face on the planet cube

    // Render Data
    VGVertexArray m_vao = 0;
    VGVertexBuffer m_vbo = 0;
    int m_numVertices = 0;
};

class ROAMPlanet {
public:
    void init(f64 diameter) {
        m_diameter = diameter;

        f64 patchWidth = m_diameter / ROAM_PATCH_DIAMETER;

        // TODO(Ben): More than 1 face
        for (int i = 0; i < 1; i++) {
            for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
                for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                    m_patches[i][y][x].init(f64v2(x * patchWidth, y * patchWidth) - ROAM_PATCH_RADIUS * patchWidth, FACE_TOP, patchWidth);
                    m_patches[i][y][x].tesselate();
                }
            }
        }
    }

    void update() {
        // TODO(Ben): More than 1 face
        for (int i = 0; i < 1; i++) {
            for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
                for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                    m_patches[i][y][x].tesselate();
                }
            }
        }
    }

    void render() {
        // TODO(Ben): More than 1 face
        for (int i = 0; i < 1; i++) {
            for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
                for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                    m_patches[i][y][x].render();
                }
            }
        }
    }

private:
    ROAMPatch m_patches[6][ROAM_PATCH_DIAMETER][ROAM_PATCH_DIAMETER];
    f64 m_diameter;
};

#endif // ROAMTerrainRenderer_h__
