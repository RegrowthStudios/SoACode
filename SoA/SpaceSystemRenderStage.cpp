#include "stdafx.h"
#include "SpaceSystem.h"
#include "SpaceSystemRenderStage.h"


SpaceSystemRenderStage::SpaceSystemRenderStage(const SpaceSystem* spaceSystem,
                                               const Camera* camera,
                                               vg::GLProgram* colorProgram) :
    m_spaceSystem(spaceSystem),
    m_camera(camera),
    m_colorProgram(colorProgram) {
    // Empty
}


SpaceSystemRenderStage::~SpaceSystemRenderStage() {
    // Empty
}

void SpaceSystemRenderStage::draw() {

    //isn't const so we have to cast it
    const_cast<SpaceSystem*>(m_spaceSystem)->drawBodies(m_camera);
    const_cast<SpaceSystem*>(m_spaceSystem)->drawPaths(m_camera, m_colorProgram);
}