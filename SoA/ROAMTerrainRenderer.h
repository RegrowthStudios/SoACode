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
#include <Vorb/graphics/GLProgram.h>
#include "VoxelCoordinateSpaces.h"
#include "Camera.h"

// Depth of variance tree: should be near SQRT(PATCH_SIZE) + 1
#define VARIANCE_DEPTH (10)
const int NUM_NODES = 1 << VARIANCE_DEPTH;

// TODO(Ben): Vary this
// Diameter of a planet cube in patches
#define ROAM_PATCH_DIAMETER (4)
#define ROAM_PATCH_RADIUS (2)

// How many TriTreeNodes should be allocated?
#define POOL_SIZE (25000)

class ROAMPlanet;

/// Vertex for terrain patch
struct ROAMTerrainVertex {
    f32v3     position;    
    f32v3     normal;      
    ColorRGB8 color;       
    ui8       padding;     
    ui8       temperature; 
    ui8       humidity;    
    ui8       padding2[2];
};
static_assert(sizeof(ROAMTerrainVertex) == 32, "ROAMTerrainVertex is not 32");

struct TmpROAMTerrainVertex {
    TmpROAMTerrainVertex() {}

    f32v3 position;
    ui8v4 color;
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
    void init(const ROAMPlanet* parent, const f64v2& gridPosition, WorldCubeFace cubeFace, f64 width);
    void reset();

    void computeVariance();
    void tesselate(const f64v3& relCamPos);

    void render(const Camera* camera, const vg::GLProgram& program, const f64v3& relativePos);

private:
    void recursTessellate(ROAMTriTreeNode * tri,
                          f64v2 left,
                          f64v2 right,
                          f64v2 apex,
                          int node,
                          const f64v3& relCamPos);

    void split(ROAMTriTreeNode * tri);

    void generateMesh();
    void recursGenerateMesh(ROAMTriTreeNode *tri, f64v2 leftX, f64v2 right, f64v2 apex);

    ui8 recursComputeVariance(f64v3 left,
                              f64v3 right,
                              f64v3 apex,
                              int node);


    f64v2 m_gridPos  = f64v2(0.0); ///< Position on 2D grid
    f64v3 m_aabbPos  = f64v3(0.0); ///< Position relative to world
    f64v3 m_aabbDims = f64v3(0.0);
    f64   m_width;

    ui8   m_varianceLeft[NUM_NODES]; // Left variance tree
    ui8   m_varianceRight[NUM_NODES]; // Right variance tree

    ui8 * m_currentVariance; // Which variance we are currently using. [Only valid during the Tessellate and ComputeVariance passes]
    bool  m_varianceDirty;   // Does the Variance Tree need to be recalculated for this Patch?
    bool  m_isVisible;       // Is this patch visible in the current frame?
    bool  m_isDirty;

    ROAMTriTreeNode m_baseLeft;  // Left base triangle tree node
    ROAMTriTreeNode m_baseRight; // Right base triangle tree node

    WorldCubeFace m_cubeFace;  // Face on the planet cube

    // Render Data
    VGVertexArray m_vao = 0;
    VGVertexBuffer m_vbo = 0;
    int m_numVertices = 0;

    const ROAMPlanet* m_parent = nullptr;
};

class ROAMPlanet {
public:
    friend class ROAMPatch;

    void init(f64 diameter) {
        m_diameter = diameter;

        f64 patchWidth = m_diameter / ROAM_PATCH_DIAMETER;

        for (int i = 0; i < 6; i++) {
            for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
                for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                    m_patches[i][y][x].init(this, f64v2(x * patchWidth, y * patchWidth) - ROAM_PATCH_RADIUS * patchWidth, (WorldCubeFace)i, patchWidth);
                }
            }
        }
    }

        void update(const f64v3& relCamPos) {
            for (int i = 0; i < 6; i++) {
                for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
                    for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                        m_patches[i][y][x].computeVariance();
                    }
                }
            }
            for (int i = 0; i < 6; i++) {
                for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
                    for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                        m_patches[i][y][x].tesselate(relCamPos);
                    }
                }
            }
    }

    void render(const Camera* camera, const f64v3& relativePos);

private:
    vg::GLProgram m_program;

    ROAMPatch m_patches[6][ROAM_PATCH_DIAMETER][ROAM_PATCH_DIAMETER];
    f64 m_diameter;
};

#endif // ROAMTerrainRenderer_h__
