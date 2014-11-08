#include "stdafx.h"
#include "MainMenuRenderPipeline.h"

#include "Errors.h"
#include "SkyboxRenderStage.h"
#include "PlanetRenderStage.h"
#include "AwesomiumRenderStage.h"
#include "HdrRenderStage.h"
#include "Options.h"

MainMenuRenderPipeline::MainMenuRenderPipeline() {
    // Empty
}


MainMenuRenderPipeline::~MainMenuRenderPipeline() {
    destroy();
}

void MainMenuRenderPipeline::init(const ui32v4& viewport, Camera* camera, IAwesomiumInterface* awesomiumInterface, vg::GLProgramManager* glProgramManager) {
    // Set the viewport
    _viewport = viewport;

    // Check to make sure we aren't leaking memory
    if (_skyboxRenderStage != nullptr) {
        pError("Reinitializing MainMenuRenderPipeline without first calling destroy()!");
    }

    // Construct framebuffer
    _hdrFrameBuffer = new vg::GLRenderTarget(_viewport.z, _viewport.w);
    _hdrFrameBuffer->init(vg::TextureInternalFormat::RGBA16F, graphicsOptions.msaa).initDepth();
    if (graphicsOptions.msaa > 0) {
        glEnable(GL_MULTISAMPLE);
    } else {
        glDisable(GL_MULTISAMPLE);
    }

    // Make swap chain
    _swapChain = new vg::RTSwapChain<2>(_viewport.z, _viewport.w);
    _swapChain->init(vg::TextureInternalFormat::RGBA8);
    _quad.init();

    // Init render stages
    _skyboxRenderStage = new SkyboxRenderStage(glProgramManager->getProgram("Texture"), camera);
    _planetRenderStage = new PlanetRenderStage(camera);
    _awesomiumRenderStage = new AwesomiumRenderStage(awesomiumInterface, glProgramManager->getProgram("Texture2D"));
    _hdrRenderStage = new HdrRenderStage(glProgramManager->getProgram("HDR"), &_quad);
}

void MainMenuRenderPipeline::render() {
 
    // Bind the FBO
    _hdrFrameBuffer->use();
    // Clear depth buffer. Don't have to clear color since skybox will overwrite it
    glClear(GL_DEPTH_BUFFER_BIT);

    // Main render passes
    _skyboxRenderStage->draw();
    _planetRenderStage->draw();
    _awesomiumRenderStage->draw();

    // Post processing
    _swapChain->reset(0, _hdrFrameBuffer, graphicsOptions.msaa > 0, false);

    // TODO: More Effects?

    // Draw to backbuffer for the last effect
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDrawBuffer(GL_BACK);
    glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
    _hdrRenderStage->draw();

    // Check for errors, just in case
    checkGlError("MainMenuRenderPipeline::render()");
}

void MainMenuRenderPipeline::destroy() {
    delete _skyboxRenderStage;
    _skyboxRenderStage = nullptr;

    delete _planetRenderStage;
    _planetRenderStage = nullptr;

    delete _awesomiumRenderStage;
    _awesomiumRenderStage = nullptr;

    delete _hdrRenderStage;
    _hdrRenderStage = nullptr;

    _hdrFrameBuffer->dispose();
    delete _hdrFrameBuffer;
    _hdrFrameBuffer = nullptr;

    _swapChain->dispose();
    delete _swapChain;
    _swapChain = nullptr;

    _quad.dispose();
}
