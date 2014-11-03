#include "stdafx.h"
#include "GamePlayScreen.h"

#include "Player.h"
#include "FrameBuffer.h"
#include "App.h"
#include "GameManager.h"
#include "InputManager.h"
#include "Sound.h"
#include "MessageManager.h"
#include "Planet.h"
#include "TerrainPatch.h"
#include "MeshManager.h"
#include "ChunkMesh.h"
#include "ParticleMesh.h"
#include "PhysicsBlocks.h"
#include "RenderTask.h"
#include "Errors.h"
#include "ChunkRenderer.h"
#include "ChunkManager.h"
#include "VoxelWorld.h"
#include "VoxelEditor.h"
#include "DebugRenderer.h"
#include "Collision.h"
#include "Inputs.h"
#include "TexturePackLoader.h"
#include "LoadTaskShaders.h"
#include "GpuMemory.h"
#include "SpriteFont.h"
#include "SpriteBatch.h"
#include "colors.h"
#include "Options.h"

#define THREAD ThreadId::UPDATE

CTOR_APP_SCREEN_DEF(GamePlayScreen, App),
    _updateThread(nullptr),
    _threadRunning(false), 
    _inFocus(true) {
    // Empty
}

i32 GamePlayScreen::getNextScreen() const {
    return SCREEN_INDEX_NO_SCREEN;
}

i32 GamePlayScreen::getPreviousScreen() const {
    return SCREEN_INDEX_NO_SCREEN;
}

void GamePlayScreen::build(){

}

void GamePlayScreen::destroy(const GameTime& gameTime) {

}

void GamePlayScreen::onEntry(const GameTime& gameTime) {

    _player = GameManager::player;
    _player->initialize("Ben", _app->getWindow().getAspectRatio()); //What an awesome name that is
    GameManager::initializeVoxelWorld(_player);

    // Initialize the PDA
    _pda.init(this);

    // Set up the rendering
    initRenderPipeline();

    // Initialize and run the update thread
    _updateThread = new std::thread(&GamePlayScreen::updateThreadFunc, this);

    // Force him to fly... This is temporary
    _player->isFlying = true;

    SDL_SetRelativeMouseMode(SDL_TRUE);

}

void GamePlayScreen::onExit(const GameTime& gameTime) {
    _threadRunning = false;
    _updateThread->join();
    delete _updateThread;
    _app->meshManager->destroy();
    _pda.destroy();
    _renderPipeline.destroy();
}

void GamePlayScreen::onEvent(const SDL_Event& e) {

    // Push the event to the input manager
    GameManager::inputManager->pushEvent(e);

    // Push event to PDA if its open
    if (_pda.isOpen()) {
        _pda.onEvent(e);
    }

    // Handle custom input
    switch (e.type) {
        case SDL_MOUSEMOTION:
            if (_inFocus) {
                // Pass mouse motion to the player
                _player->mouseMove(e.motion.xrel, e.motion.yrel);
            }
            break;
        case SDL_MOUSEBUTTONDOWN:
            onMouseDown(e);
            break;
        case SDL_MOUSEBUTTONUP:
            onMouseUp(e);
            break;
        case SDL_WINDOWEVENT:
            if (e.window.type == SDL_WINDOWEVENT_LEAVE || e.window.type == SDL_WINDOWEVENT_FOCUS_LOST){
                 SDL_SetRelativeMouseMode(SDL_FALSE);
                 _inFocus = false;
                 SDL_StopTextInput();
            } else if (e.window.type == SDL_WINDOWEVENT_ENTER) {
                SDL_SetRelativeMouseMode(SDL_TRUE);
                _inFocus = true;
                SDL_StartTextInput();
            }
        default:
            break;
    }
}

void GamePlayScreen::update(const GameTime& gameTime) {

    // TEMPORARY TIMESTEP TODO(Ben): Get rid of this damn global
    if (_app->getFps()) {
        glSpeedFactor = 60.0f / _app->getFps();
        if (glSpeedFactor > 3.0f) { // Cap variable timestep at 20fps
            glSpeedFactor = 3.0f;
        }
    }
    
    // Update the input
    handleInput();

    // Update the player
    updatePlayer();

    // Update the PDA
    _pda.update();

    // Sort all meshes // TODO(Ben): There is redundancy here
    _app->meshManager->sortMeshes(_player->headPosition);

    // Process any updates from the render thread
    processMessages();
}

void GamePlayScreen::draw(const GameTime& gameTime) {

    updateWorldCameraClip();

    _renderPipeline.render();
}

i32 GamePlayScreen::getWindowWidth() const {
    return _app->getWindow().getWidth();
}

i32 GamePlayScreen::getWindowHeight() const {
    return _app->getWindow().getHeight();
}

void GamePlayScreen::initRenderPipeline() {
    // Set up the rendering pipeline and pass in dependencies
    ui32v4 viewport(0, 0, _app->getWindow().getViewportDims());
    _renderPipeline.init(viewport, &_player->getChunkCamera(), &_player->getWorldCamera(), 
                         _app, _player, _app->meshManager, &_pda, GameManager::glProgramManager);
}

void GamePlayScreen::handleInput() {
    // Get input manager handle
    InputManager* inputManager = GameManager::inputManager;
    // Handle key inputs
    if (inputManager->getKeyDown(INPUT_PAUSE)) {
        SDL_SetRelativeMouseMode(SDL_FALSE);
        _inFocus = false;
    }
    if (inputManager->getKeyDown(INPUT_FLY)) {
        _player->flyToggle();
    }
    if (inputManager->getKeyDown(INPUT_GRID)) {
        gridState = !gridState;
    }
    if (inputManager->getKeyDown(INPUT_RELOAD_TEXTURES)) {
        // Free atlas
        vg::GpuMemory::freeTexture(blockPack.textureInfo.ID);
        // Free all textures
        GameManager::textureCache->destroy();
        // Reload textures
        GameManager::texturePackLoader->loadAllTextures("Textures/TexturePacks/" + graphicsOptions.texturePackString + "/");
        GameManager::texturePackLoader->uploadTextures();
        GameManager::texturePackLoader->writeDebugAtlases();
        GameManager::texturePackLoader->setBlockTextures(Blocks);

        GameManager::getTextureHandles();

        // Initialize all the textures for blocks.
        for (size_t i = 0; i < Blocks.size(); i++) {
            Blocks[i].InitializeTexture();
        }

        GameManager::texturePackLoader->destroy();
    }
    if (inputManager->getKeyDown(INPUT_RELOAD_SHADERS)) {
        GameManager::glProgramManager->destroy();
        LoadTaskShaders shaderTask;
        shaderTask.load();
        // Need to reinitialize the render pipeline with new shaders
        _renderPipeline.destroy();
        initRenderPipeline();
    }
    if (inputManager->getKeyDown(INPUT_INVENTORY)) {
        if (_pda.isOpen()) {
            _pda.close();
            SDL_SetRelativeMouseMode(SDL_TRUE);
            _inFocus = true;
            SDL_StartTextInput();
        } else {
            _pda.open();
            SDL_SetRelativeMouseMode(SDL_FALSE);
            _inFocus = false;
            SDL_StopTextInput();
        }
    }
    if (inputManager->getKeyDown(INPUT_RELOAD_UI)) {
        if (_pda.isOpen()) {
            _pda.close();
        }
        _pda.destroy();
        _pda.init(this);
    }

    // Block placement
    if (!_pda.isOpen()) {
        if (inputManager->getKeyDown(INPUT_MOUSE_LEFT) || (GameManager::voxelEditor->isEditing() && inputManager->getKey(INPUT_BLOCK_DRAG))) {
            if (!(_player->leftEquippedItem)){
                GameManager::clickDragRay(true);
            } else if (_player->leftEquippedItem->type == ITEM_BLOCK){
                _player->dragBlock = _player->leftEquippedItem;
                GameManager::clickDragRay(false);
            }
        } else if (inputManager->getKeyDown(INPUT_MOUSE_RIGHT) || (GameManager::voxelEditor->isEditing() && inputManager->getKey(INPUT_BLOCK_DRAG))) {
            if (!(_player->rightEquippedItem)){
                GameManager::clickDragRay(true);
            } else if (_player->rightEquippedItem->type == ITEM_BLOCK){
                _player->dragBlock = _player->rightEquippedItem;
                GameManager::clickDragRay(false);
            }
        }
    }

    // Dev hud
    if (inputManager->getKeyDown(INPUT_HUD)) {
        _renderPipeline.cycleDevHud();
    }

    // Update inputManager internal state
    inputManager->update();
}

void GamePlayScreen::onMouseDown(const SDL_Event& e) {
    SDL_StartTextInput();
    if (!_pda.isOpen()) {
        SDL_SetRelativeMouseMode(SDL_TRUE);
        _inFocus = true;
    }
}

void GamePlayScreen::onMouseUp(const SDL_Event& e) {
    if (e.button.button == SDL_BUTTON_LEFT) {
        if (GameManager::voxelEditor->isEditing()) {
            GameManager::voxelEditor->editVoxels(_player->leftEquippedItem);
        }
    } else if (e.button.button == SDL_BUTTON_RIGHT) {
        if (GameManager::voxelEditor->isEditing()) {
            GameManager::voxelEditor->editVoxels(_player->rightEquippedItem);
        }
    }
}

void GamePlayScreen::updatePlayer() {
    double dist = _player->facePosition.y + GameManager::planet->radius;
    _player->update(_inFocus, GameManager::planet->getGravityAccel(dist), GameManager::planet->getAirFrictionForce(dist, glm::length(_player->velocity)));

    Chunk **chunks = new Chunk*[8];
    _player->isGrounded = 0;
    _player->setMoveMod(1.0f);
    _player->canCling = 0;
    _player->collisionData.yDecel = 0.0f;
    //    cout << "C";

    // Number of steps to integrate the collision over
    Chunk::modifyLock.lock();
    for (int i = 0; i < PLAYER_COLLISION_STEPS; i++){
        _player->gridPosition += (_player->velocity / (float)PLAYER_COLLISION_STEPS) * glSpeedFactor;
        _player->facePosition += (_player->velocity / (float)PLAYER_COLLISION_STEPS) * glSpeedFactor;
        _player->collisionData.clear();
        GameManager::voxelWorld->getClosestChunks(_player->gridPosition, chunks); //DANGER HERE!
        aabbChunkCollision(_player, &(_player->gridPosition), chunks, 8);
        _player->applyCollisionData();
    }
    Chunk::modifyLock.unlock();

    delete[] chunks;
}

void GamePlayScreen::updateThreadFunc() {
    _threadRunning = true;

    FpsLimiter fpsLimiter;
    fpsLimiter.init(maxPhysicsFps);

    MessageManager* messageManager = GameManager::messageManager;

    Message message;

    while (_threadRunning) {
        fpsLimiter.beginFrame();

        GameManager::soundEngine->SetMusicVolume(soundOptions.musicVolume / 100.0f);
        GameManager::soundEngine->SetEffectVolume(soundOptions.effectVolume / 100.0f);
        GameManager::soundEngine->update();

        while (messageManager->tryDeque(THREAD, message)) {
            // Process the message
            switch (message.id) {
                
            }
        }

        f64v3 camPos = glm::dvec3((glm::dmat4(GameManager::planet->invRotationMatrix)) * glm::dvec4(_player->getWorldCamera().getPosition(), 1.0));

        GameManager::update();

        physicsFps = fpsLimiter.endFrame();
    }
}

void GamePlayScreen::processMessages() {

    TerrainMeshMessage* tmm;
    Message message;

    MeshManager* meshManager = _app->meshManager;

    while (GameManager::messageManager->tryDeque(ThreadId::RENDERING, message)) {
        switch (message.id) {
            case MessageID::TERRAIN_MESH:
                meshManager->updateTerrainMesh(static_cast<TerrainMeshMessage*>(message.data));
                break;
            case MessageID::REMOVE_TREES:
                tmm = static_cast<TerrainMeshMessage*>(message.data);
                if (tmm->terrainBuffers->treeVboID != 0) glDeleteBuffers(1, &(tmm->terrainBuffers->treeVboID));
                tmm->terrainBuffers->treeVboID = 0;
                delete tmm;
                break;
            case MessageID::CHUNK_MESH:
                meshManager->updateChunkMesh((ChunkMeshData *)(message.data));
                break;
            case MessageID::PARTICLE_MESH:
                meshManager->updateParticleMesh((ParticleMeshMessage *)(message.data));
                break;
            case MessageID::PHYSICS_BLOCK_MESH:
                meshManager->updatePhysicsBlockMesh((PhysicsBlockMeshMessage *)(message.data));
                break;
            default:
                break;
        }
    }
}

void GamePlayScreen::updateWorldCameraClip() {
    //far znear for maximum Terrain Patch z buffer precision
    //this is currently incorrect
    double nearClip = MIN((csGridWidth / 2.0 - 3.0)*32.0*0.7, 75.0) - ((double)(GameManager::chunkIOManager->getLoadListSize()) / (double)(csGridWidth*csGridWidth*csGridWidth))*55.0;
    if (nearClip < 0.1) nearClip = 0.1;
    double a = 0.0;
    // TODO(Ben): This is crap fix it (Sorry Brian)
    a = closestTerrainPatchDistance / (sqrt(1.0f + pow(tan(graphicsOptions.fov / 2.0), 2.0) * (pow((double)_app->getWindow().getAspectRatio(), 2.0) + 1.0))*2.0);
    if (a < 0) a = 0;

    double clip = MAX(nearClip / planetScale * 0.5, a);
    // The world camera has a dynamic clipping plane
    _player->getWorldCamera().setClippingPlane(clip, MAX(300000000.0 / planetScale, closestTerrainPatchDistance + 10000000));
    _player->getWorldCamera().updateProjection();
}
