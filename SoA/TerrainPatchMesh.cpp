#include "stdafx.h"
#include "TerrainPatchMesh.h"

#include <Vorb/TextureRecycler.hpp>
#include <Vorb/graphics/GLProgram.h>
#include <Vorb/graphics/GpuMemory.h>

#include "Camera.h"
#include "RenderUtils.h"
#include "soaUtils.h"
#include "VoxelCoordinateSpaces.h"
#include "VoxelSpaceConversions.h"

TerrainPatchMesh::~TerrainPatchMesh() {
    if (m_vbo) {
        vg::GpuMemory::freeBuffer(m_vbo);
    }
    if (m_wvbo) {
        vg::GpuMemory::freeBuffer(m_wvbo);
    }
    if (m_wibo) {
        vg::GpuMemory::freeBuffer(m_wibo);
    }
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
    }
    if (m_wvao) {
        glDeleteVertexArrays(1, &m_wvao);
    }
}

void TerrainPatchMesh::draw(const f32m4& WVP, const vg::GLProgram& program,
                            bool drawSkirts) const {
    glUniformMatrix4fv(program.getUniform("unWVP"), 1, GL_FALSE, &WVP[0][0]);

    glBindVertexArray(m_vao);

    if (drawSkirts) {
        glDrawElements(GL_TRIANGLES, PATCH_INDICES, GL_UNSIGNED_SHORT, 0);
    } else {
        glDrawElements(GL_TRIANGLES, PATCH_INDICES_NO_SKIRTS, GL_UNSIGNED_SHORT, 0);
    }
    glBindVertexArray(0);
}

void TerrainPatchMesh::drawWater(const f32m4& WVP, const vg::GLProgram& program) const {
    glUniformMatrix4fv(program.getUniform("unWVP"), 1, GL_FALSE, &WVP[0][0]);

    glBindVertexArray(m_wvao);

    glDrawElements(GL_TRIANGLES, m_waterIndexCount, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
}

void TerrainPatchMesh::drawAsFarTerrain(const f64v3& relativePos, const f32m4& VP,
                                        const vg::GLProgram& program,
                                        bool drawSkirts) const {
    // No need for matrix with simple translation
    f32v3 translation = f32v3(f64v3(m_aabbPos) - relativePos);

    glUniformMatrix4fv(program.getUniform("unVP"), 1, GL_FALSE, &VP[0][0]);
    glUniform3fv(program.getUniform("unTranslation"), 1, &translation[0]);
    glUniform3fv(program.getUniform("unPosition"), 1, &m_aabbPos[0]);

    glBindVertexArray(m_vao);

    if (drawSkirts) {
        glDrawElements(GL_TRIANGLES, PATCH_INDICES, GL_UNSIGNED_SHORT, 0);
    } else {
        glDrawElements(GL_TRIANGLES, PATCH_INDICES_NO_SKIRTS, GL_UNSIGNED_SHORT, 0);
    }

    glBindVertexArray(0);
}

/// Draws the water mesh as a far terrain mesh
void TerrainPatchMesh::drawWaterAsFarTerrain(const f64v3& relativePos, const f32m4& VP,
                                             const vg::GLProgram& program) const {
    // No need for matrix with simple translation
    f32v3 translation = f32v3(f64v3(m_aabbPos) - relativePos);

    glUniformMatrix4fv(program.getUniform("unVP"), 1, GL_FALSE, &VP[0][0]);
    glUniform3fv(program.getUniform("unTranslation"), 1, &translation[0]);
    glUniform3fv(program.getUniform("unPosition"), 1, &m_aabbPos[0]);

    glBindVertexArray(m_wvao);

    glDrawElements(GL_TRIANGLES, m_waterIndexCount, GL_UNSIGNED_SHORT, 0);

    glBindVertexArray(0);
}

f32v3 TerrainPatchMesh::getClosestPoint(const f32v3& camPos) const {
    return getClosestPointOnAABB(camPos, m_aabbPos, m_aabbDims);
}
f64v3 TerrainPatchMesh::getClosestPoint(const f64v3& camPos) const {
    return getClosestPointOnAABB(camPos, f64v3(m_aabbPos), f64v3(m_aabbDims));
}
