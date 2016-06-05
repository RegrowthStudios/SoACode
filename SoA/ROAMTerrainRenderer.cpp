#include "stdafx.h"

#include "ROAMTerrainRenderer.h"

int ROAMPatchRecycler::m_nextTriNode = 0;

std::vector<TmpROAMTerrainVertex> vertices; // TMP

void ROAMPatch::init(const f64v2& gridPosition, WorldCubeFace cubeFace, f64 width) {
    m_gridPos = gridPosition;
    m_cubeFace = cubeFace;
    m_width = width;

    // Attach the two m_base triangles together
    m_baseLeft.baseNeighbor = &m_baseRight;
    m_baseRight.baseNeighbor = &m_baseLeft;

    // Initialize flags
    m_varianceDirty = 1;
    m_isVisible = 0;
}

void ROAMPatch::reset() {
    // Assume patch is not visible.
    m_isVisible = 0;

    // Reset the important relationships
    memset(&m_baseLeft, 0, sizeof(ROAMTriTreeNode));
    memset(&m_baseRight, 0, sizeof(ROAMTriTreeNode));

    // Attach the two m_base triangles together
    m_baseLeft.baseNeighbor = &m_baseRight;
    m_baseRight.baseNeighbor = &m_baseLeft;
}

void ROAMPatch::tesselate() {
    // Split each of the base triangles
    m_currentVariance = m_varianceLeft;
    recursTessellate(&m_baseLeft,
                     f64v2(m_gridPos.x, m_gridPos.y + m_width),
                     f64v2(m_gridPos.x + m_width, m_gridPos.y),
                     m_gridPos,
                     1);

    m_currentVariance = m_varianceRight;
    recursTessellate(&m_baseRight,
                     f64v2(m_gridPos.x + m_width, m_gridPos.y),
                     f64v2(m_gridPos.x, m_gridPos.y + m_width),
                     f64v2(m_gridPos.x + m_width, m_gridPos.y + m_width),
                     1);
}

void ROAMPatch::render() {

    // TODO(Ben): Camera

    if (m_isDirty) {
        generateMesh();
    }

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    glDrawArrays(GL_TRIANGLES, 0, m_numVertices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ROAMPatch::recursTessellate(ROAMTriTreeNode * tri, f64v2 left, f64v2 right, f64v2 apex, int node) {
    f64 TriVariance;
    f64v2 center = (left + right) * 2.0; // Compute X coordinate of center of Hypotenuse

    if (node < (1 << VARIANCE_DEPTH)) {
        // Extremely slow distance metric (sqrt is used).
        // Replace this with a faster one!
        f64 distance = 1000; // 1.0 + sqrt(pow((f64)centerX - gViewPosition[0], 2) + pow((f64)centerY - gViewPosition[2], 2));

        // Egads!  A division too?  What's this world coming to!
        // This should also be replaced with a faster operation.
        TriVariance = ((float)m_currentVariance[node] * 2048 * 2) / distance;	// Take both distance and variance into consideration
    }
    // Else?
    if ((node >= (1 << VARIANCE_DEPTH)) ||	// IF we do not have variance info for this node, then we must have gotten here by splitting, so continue down to the lowest level.
        (TriVariance > 1000.0f))	// OR if we are not below the variance tree, test for variance.
    {
        split(tri);														// Split this triangle.

        if (tri->leftChild &&											// If this triangle was split, try to split it's children as well.
            ((abs(left.x - right.x) >= 3) || (abs(left.y - right.y) >= 3)))	// Tessellate all the way down to one vertex per height field entry
        {
            recursTessellate(tri->leftChild, apex, left, center, node << 1);
            recursTessellate(tri->leftChild, right, apex, center, 1 + (node << 1));
        }
    }
}

void ROAMPatch::split(ROAMTriTreeNode * tri) {
    // We are already split, no need to do it again.
    if (tri->leftChild)
        return;

    // If this triangle is not in a proper diamond, force split our base neighbor
    if (tri->baseNeighbor && (tri->baseNeighbor->baseNeighbor != tri))
        split(tri->baseNeighbor);

    // Create children and link into mesh
    tri->leftChild = ROAMPatchRecycler::AllocateTri();
    tri->rightChild = ROAMPatchRecycler::AllocateTri();

    // If creation failed, just exit. This sucks
    if (!tri->rightChild)
        return;

    // Fill in the information we can get from the parent (neighbor pointers)
    tri->leftChild->baseNeighbor = tri->leftNeighbor;
    tri->leftChild->leftNeighbor = tri->rightChild;

    tri->rightChild->baseNeighbor = tri->rightNeighbor;
    tri->rightChild->rightNeighbor = tri->leftChild;

    // Link our left Neighbor to the new children
    if (tri->leftNeighbor != NULL) {
        if (tri->leftNeighbor->baseNeighbor == tri)
            tri->leftNeighbor->baseNeighbor = tri->leftChild;
        else if (tri->leftNeighbor->leftNeighbor == tri)
            tri->leftNeighbor->leftNeighbor = tri->leftChild;
        else if (tri->leftNeighbor->rightNeighbor == tri)
            tri->leftNeighbor->rightNeighbor = tri->leftChild;
        else
            throw(142);// Illegal left Neighbor!
    }

    // Link our right Neighbor to the new children
    if (tri->rightNeighbor != NULL) {
        if (tri->rightNeighbor->baseNeighbor == tri)
            tri->rightNeighbor->baseNeighbor = tri->rightChild;
        else if (tri->rightNeighbor->rightNeighbor == tri)
            tri->rightNeighbor->rightNeighbor = tri->rightChild;
        else if (tri->rightNeighbor->leftNeighbor == tri)
            tri->rightNeighbor->leftNeighbor = tri->rightChild;
        else
            throw(143);// Illegal right Neighbor!
    }

    // Link our base Neighbor to the new children
    if (tri->baseNeighbor != NULL) {
        if (tri->baseNeighbor->leftChild) {
            tri->baseNeighbor->leftChild->rightNeighbor = tri->rightChild;
            tri->baseNeighbor->rightChild->leftNeighbor = tri->leftChild;
            tri->leftChild->rightNeighbor = tri->baseNeighbor->rightChild;
            tri->rightChild->leftNeighbor = tri->baseNeighbor->leftChild;
        } else
            split(tri->baseNeighbor);  // base Neighbor (in a diamond with us) was not split yet, so do that now.
    } else {
        // An edge triangle, trivial case.
        tri->leftChild->rightNeighbor = NULL;
        tri->rightChild->leftNeighbor = NULL;
    }

    m_isDirty = true;
}

void ROAMPatch::generateMesh() {
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
    }

    recursGenerateMesh(&m_baseLeft,
                 f64v2(0, m_width),
                 f64v2(m_width, 0),
                 f64v2(0, 0));

    recursGenerateMesh(&m_baseRight,
                 f64v2(m_width, 0),
                 f64v2(0, m_width),
                 f64v2(m_width, m_width));

    m_isDirty = false;
}

void ROAMPatch::recursGenerateMesh(ROAMTriTreeNode *tri, f64v2 left, f64v2 right, f64v2 apex) {
    if (tri->leftChild)					// All non-leaf nodes have both children, so just check for one
    {
        f64v2 center = (left + right) * 2.0;

        recursGenerateMesh(tri->leftChild, apex, left, center);
        recursGenerateMesh(tri->rightChild, right, apex, center);
    } else									// A leaf node!  Output a triangle to be rendered.
    {
        // Actual number of rendered triangles...
        m_numVertices += 3;


        // Output the LEFT VERTEX for the triangle
        vertices.emplace_back((float)left.x,
                              (float)left.y,
                              0.0f);

      
        // Output the RIGHT VERTEX for the triangle
        vertices.emplace_back((float)left.x,
                              (float)left.y,
                              0.0f);


        vertices.emplace_back((float)left.x,
                              (float)left.y,
                              0.0f);
    }
}