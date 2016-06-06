#include "stdafx.h"

#include "ROAMTerrainRenderer.h"

#include "ShaderLoader.h"

#include "VoxelSpaceConversions.h"
#include "RenderUtils.h"

int ROAMPatchRecycler::m_nextTriNode = 0;
ROAMTriTreeNode ROAMPatchRecycler::m_triPool[POOL_SIZE];

// TODO(Ben): NO globals
std::vector<TmpROAMTerrainVertex> vertices; // TMP

namespace {

    const char * VERT_SRC = R"(
                          
// Uniforms
uniform mat4 unWVP;

// Input
in vec4 vPosition;
in vec3 vColor;

out vec3 fColor;

void main() {
  fColor = vColor;
  gl_Position = unWVP * vPosition;
}
)";

const char * FRAG_SRC = R"(

in vec3 fColor;
// Output
out vec4 pColor;

void main() {
  pColor = vec4(fColor.xyz, 1.0);
}

)";
}


void ROAMPatch::init(const ROAMPlanet* parent, const f64v2& gridPosition, WorldCubeFace cubeFace, f64 width) {
    m_parent = parent;
    m_gridPos = gridPosition;
    m_cubeFace = cubeFace;
    m_width = width;

    // Attach the two m_base triangles together
    m_baseLeft.baseNeighbor = &m_baseRight;
    m_baseRight.baseNeighbor = &m_baseLeft;

    // Initialize flags
    m_varianceDirty = true;
    m_isVisible = false;
    m_isDirty = true;
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

void ROAMPatch::tesselate(const f64v3& relCamPos) {
    // Split each of the base triangles
    m_currentVariance = m_varianceLeft;

    recursTessellate(&m_baseLeft,
                     f64v2(m_gridPos.x, m_gridPos.y + m_width),
                     f64v2(m_gridPos.x + m_width, m_gridPos.y),
                     m_gridPos,
                     1, relCamPos);

    m_currentVariance = m_varianceRight;
    recursTessellate(&m_baseRight,
                     f64v2(m_gridPos.x + m_width, m_gridPos.y),
                     f64v2(m_gridPos.x, m_gridPos.y + m_width),
                     f64v2(m_gridPos.x + m_width, m_gridPos.y + m_width),
                     1, relCamPos);
}

void ROAMPatch::render(const Camera* camera, const vg::GLProgram& program, const f64v3& relativePos) {

    // TODO(Ben): Camera

    if (m_isDirty) {
        generateMesh();
    }
    glDisable(GL_DEPTH_TEST);

    f32m4 W(1.0);
    setMatrixTranslation(W, -relativePos);
    f32m4 WVP = camera->getViewProjectionMatrix() * W;

    glUniformMatrix4fv(program.getUniform("unWVP"), 1, GL_FALSE, &WVP[0][0]);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TmpROAMTerrainVertex), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TmpROAMTerrainVertex), (void*)offsetof(TmpROAMTerrainVertex, color));

    glDrawArrays(GL_TRIANGLES, 0, m_numVertices);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void ROAMPatch::recursTessellate(ROAMTriTreeNode * tri, f64v2 left, f64v2 right, f64v2 apex, int node, const f64v3& relCamPos) {

    const i32v3& m_coordMapping = VoxelSpaceConversions::VOXEL_TO_WORLD[(int)m_cubeFace];
    const f32v2 m_coordMults = f32v2(VoxelSpaceConversions::FACE_TO_WORLD_MULTS[(int)m_cubeFace]);

    f64 height = m_parent->m_diameter * 0.5;

    // optimize
    f64v2 center = (left + right) * 0.5; // Compute X coordinate of center of Hypotenuse
    f64v3 center3D;
    center3D[m_coordMapping.x] = center.x * m_coordMults.x;
    center3D[m_coordMapping.y] = height * VoxelSpaceConversions::FACE_Y_MULTS[(int)m_cubeFace];
    center3D[m_coordMapping.z] = center.y * m_coordMults.y;
    center3D = vmath::normalize(center3D) * height;

    // Extremely slow distance metric (sqrt is used).
    f32 distance = vmath::length(relCamPos - center3D);

    if (node < (1 << VARIANCE_DEPTH)) {

        // optimize
        f64 tmpSizeMult = vmath::length(left - right);
        // This should also be replaced with a faster operation.
        f32 variance = ((f32)m_currentVariance[node] * m_width * 0.001) * tmpSizeMult / distance;
        if ((variance > 26.0f))	// OR if we are not below the variance tree, test for variance.
        {
            split(tri);

            if (tri->leftChild){ // If this triangle was split, try to split it's children as well.
                recursTessellate(tri->leftChild, apex, left, center, node << 1, relCamPos);
                recursTessellate(tri->rightChild, right, apex, center, 1 + (node << 1), relCamPos);
            }
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
    m_varianceDirty = true;
}

void ROAMPatch::generateMesh() {
    if (!m_vao) {
        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);
    }

    recursGenerateMesh(&m_baseLeft,
                       f64v2(m_gridPos.x, m_gridPos.y + m_width),
                       f64v2(m_gridPos.x + m_width, m_gridPos.y),
                       f64v2(m_gridPos.x, m_gridPos.y));

    recursGenerateMesh(&m_baseRight,
                       f64v2(m_gridPos.x + m_width, m_gridPos.y),
                       f64v2(m_gridPos.x, m_gridPos.y + m_width),
                       f64v2(m_gridPos.x + m_width, m_gridPos.y + m_width));


    m_numVertices = vertices.size();

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(TmpROAMTerrainVertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    std::vector<TmpROAMTerrainVertex>().swap(vertices);

    m_isDirty = false;
}

void ROAMPatch::recursGenerateMesh(ROAMTriTreeNode *tri, f64v2 left, f64v2 right, f64v2 apex) {
    
    // Move this out of recursion
    const i32v3& m_coordMapping = VoxelSpaceConversions::VOXEL_TO_WORLD[(int)m_cubeFace];
    const f32v2 m_coordMults = f32v2(VoxelSpaceConversions::FACE_TO_WORLD_MULTS[(int)m_cubeFace]);

    f32 height = m_parent->m_diameter * 0.5;

    
    if (tri->leftChild)					// All non-leaf nodes have both children, so just check for one
    {
        f64v2 center = (left + right) * 0.5;

        recursGenerateMesh(tri->leftChild, apex, left, center);
        recursGenerateMesh(tri->rightChild, right, apex, center);
    } else									// A leaf node!  Output a triangle to be rendered.
    {
        ui32 len = (ui8)(vmath::length(left - right) / 2.0);
        ui8v4 color;
        
        if (len > 255) {
            len = len - (255 - len);
            if (len > 255) len = 255;
        }
        color.r = len;
        color.g = len;
        color.b = len;
        color.a = 255;

        // Output the LEFT VERTEX for the triangle
        vertices.emplace_back();
        vertices.back().position[m_coordMapping.x] = left.x * m_coordMults.x;
        vertices.back().position[m_coordMapping.y] = height * VoxelSpaceConversions::FACE_Y_MULTS[(int)m_cubeFace];
        vertices.back().position[m_coordMapping.z] = left.y * m_coordMults.y;
        vertices.back().position = vmath::normalize(vertices.back().position) * height;
        vertices.back().color = color;
 
        // Output the RIGHT VERTEX for the triangle
        vertices.emplace_back();
        vertices.back().position[m_coordMapping.x] = right.x * m_coordMults.x;
        vertices.back().position[m_coordMapping.y] = height * VoxelSpaceConversions::FACE_Y_MULTS[(int)m_cubeFace];
        vertices.back().position[m_coordMapping.z] = right.y * m_coordMults.y;
        vertices.back().position = vmath::normalize(vertices.back().position) * height;
        vertices.back().color = color;

        // Output the APEX VERTEX for the triangle
        vertices.emplace_back();
        vertices.back().position[m_coordMapping.x] = apex.x * m_coordMults.x;
        vertices.back().position[m_coordMapping.y] = height * VoxelSpaceConversions::FACE_Y_MULTS[(int)m_cubeFace];
        vertices.back().position[m_coordMapping.z] = apex.y * m_coordMults.y;
        vertices.back().position = vmath::normalize(vertices.back().position) * height;
        vertices.back().color = color;
    }
}

void ROAMPatch::computeVariance() {

    if (m_varianceDirty) {
        // Compute variance on each of the base triangles...
        m_currentVariance = m_varianceLeft;
        recursComputeVariance(f64v3(0.0f, m_width, 0.0f),
                              f64v3(m_width, 0.0f, 0.0f),
                              f64v3(0.0f, 0.0f, 0.0f),
                              1);

        m_currentVariance = m_varianceRight;
        recursComputeVariance(f64v3(m_width, 0.0f, 0.0f),
                              f64v3(0.0f, m_width, 0.0f),
                              f64v3(m_width, m_width, 0.0f),
                              1);

        m_varianceDirty = 0;
    }
}

ui8 ROAMPatch::recursComputeVariance(f64v3 left,
                                     f64v3 right,
                                     f64v3 apex,
                                     int node) {
    //        /|\
    //      /  |  \
    //    /    |    \
    //  /      |      \
    //  ~~~~~~~*~~~~~~~  <-- Compute the X and Y coordinates of '*'
    //
    f32v3 center;
    center.x = (left.x + right.x) * 0.5;		// Compute X coordinate of center of Hypotenuse
    center.y = 0.0f;
    center.z = (left.y + right.z) * 0.5;		// Compute Y coord...

    // Get the height value at the middle of the Hypotenuse
    f32 centerZ = 0.0f;
    const f32 VARIANCE_MULT = 0.5f;

    // Variance of this triangle is the actual height at it's hypotenuse midpoint minus the interpolated height.
    // Use values passed on the stack instead of re-accessing the Height Field.
    ui8 variance = (ui8)(abs(centerZ - ((left.y + right.y) * 0.5f)) * VARIANCE_MULT);

    // Since we're after speed and not perfect representations,
    //    only calculate variance down to an 8x8 block
    if (node < (1 << (VARIANCE_DEPTH + 1))) {
        // Final Variance for this node is the max of it's own variance and that of it's children.
        variance = vmath::max(variance, recursComputeVariance(apex, left, center, node << 1));
        variance = vmath::max(variance, recursComputeVariance(right, apex, center, 1 + (node << 1)));
    }

    // Store the final variance for this node.  Note Variance is never zero.
    if (node < (1 << VARIANCE_DEPTH))
        m_currentVariance[node] = 1 + variance;

    return variance;
}

void ROAMPlanet::render(const Camera* camera, const f64v3& relativePos) {

    // Lazy load shader
    if (!m_program.isLinked()) {
        m_program = ShaderLoader::createProgram("ROAMPlanet", VERT_SRC, FRAG_SRC);
    }

    m_program.use();
    m_program.enableVertexAttribArrays();

    for (int i = 0; i < 6; i++) {
        for (int y = 0; y < ROAM_PATCH_DIAMETER; y++) {
            for (int x = 0; x < ROAM_PATCH_DIAMETER; x++) {
                m_patches[i][y][x].render(camera, m_program, relativePos);
            }
        }
    }

    m_program.disableVertexAttribArrays();
    m_program.unuse();
}
