#include "stdafx.h"
#include "OrbitRenderer.h"

#include <Vorb/graphics/GLProgram.h>
#include <Vorb/graphics/GpuMemory.h>
#include <Vorb/utils.h>

#include "Constants.h"
#include "RenderUtils.h"
#include "SpaceSystemComponents.h"
#include "Errors.h"
#include "SpaceBodyComponentUpdater.h"

void OrbitRenderer::drawPath(const SpaceBodyComponent& cmp, OrbitPathRenderData& renderData,
                             vg::GLProgram& colorProgram, const f32m4& WVP,
                             const f64v3& camPos,
                             const color4& color, const SpaceBodyComponent* parentCmp /*= nullptr*/) {
   
    if (color.a == 0) return;

    // Lazily generate mesh
    if (renderData.vbo == 0) generateOrbitEllipse(cmp, renderData, colorProgram);

    if (renderData.numVerts == 0) return;
    
    f32m4 w(1.0f);
    if (parentCmp) {
        setMatrixTranslation(w, parentCmp->position - camPos);
    } else {
        setMatrixTranslation(w, -camPos);
    }
    
    f32m4 pathMatrix = WVP * w;

    f32v4 colorf(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);

    glUniform4f(colorProgram.getUniform("unColor"), colorf.r, colorf.g, colorf.b, colorf.a);

    glUniformMatrix4fv(colorProgram.getUniform("unWVP"), 1, GL_FALSE, &pathMatrix[0][0]);

    f32 currentAngle = (f32)((cmp.currentMeanAnomaly - cmp.startMeanAnomaly) / M_2_PI);
    glUniform1f(colorProgram.getUniform("currentAngle"), currentAngle);

    // Draw the ellipse
    glDepthMask(false);
    glBindVertexArray(renderData.vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderData.vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glDrawArrays(GL_LINE_STRIP, 0, renderData.numVerts);

    glBindVertexArray(0);
    glDepthMask(true);

}

void OrbitRenderer::generateOrbitEllipse(const SpaceBodyComponent& cmp, OrbitPathRenderData& renderData, vg::GLProgram& colorProgram) {

    std::vector <OrbitPathRenderData::Vertex> verts;
    { // Generate the mesh
        static const int NUM_VERTS = 3000;
        verts.resize(NUM_VERTS + 1);
        f64 timePerDeg = cmp.t / (f64)NUM_VERTS;

        SpaceBodyComponentUpdater updater;
        for (int i = 0; i < NUM_VERTS; i++) {

            f64v3 position;
            f64v3 tmp1;
            f64 tmp2;

            // Sample positions for the vertices
            // TODO(Ben): We can do this much more cheaply and without the updater
            // by just using semi-major and semi-minor
            updater.getPositionAndVelocity(cmp, nullptr, i * timePerDeg, position, tmp1, tmp2);

            OrbitPathRenderData::Vertex& vert = verts[i];
            vert.position = position;
            vert.angle = 1.0f - (f32)i / (f32)NUM_VERTS;
        }
        verts.back() = verts.front();
    }

    glGenVertexArrays(1, &renderData.vao);
    glBindVertexArray(renderData.vao);

    colorProgram.enableVertexAttribArrays();

    // Upload the buffer data
    vg::GpuMemory::createBuffer(renderData.vbo);
    vg::GpuMemory::bindBuffer(renderData.vbo, vg::BufferTarget::ARRAY_BUFFER);
    vg::GpuMemory::uploadBufferData(renderData.vbo,
                                    vg::BufferTarget::ARRAY_BUFFER,
                                    verts.size() * sizeof(OrbitPathRenderData::Vertex),
                                    verts.data(),
                                    vg::BufferUsageHint::STATIC_DRAW);
    vg::GpuMemory::bindBuffer(0, vg::BufferTarget::ELEMENT_ARRAY_BUFFER);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(OrbitPathRenderData::Vertex), (const void*)offsetof(OrbitPathRenderData::Vertex, position));
    glVertexAttribPointer(1, 1, GL_FLOAT, GL_FALSE, sizeof(OrbitPathRenderData::Vertex), (const void*)offsetof(OrbitPathRenderData::Vertex, angle));
    glBindVertexArray(0);
    renderData.numVerts = verts.size();
}