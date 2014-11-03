// 
//  GamePlayScreen.h
//  Seed Of Andromeda
//
//  Created by Ben Arnold on 17 Oct 2014
//  Copyright 2014 Regrowth Studios
//  All Rights Reserved
//  
//  This file provides the implementation for the main
//  gameplay screen. This will encompass all in game
//  behavior.
//

#pragma once

#ifndef GAMEPLAYSCREEN_H_
#define GAMEPLAYSCREEN_H_

#include "IGameScreen.h"

#include "AwesomiumInterface.h"
#include "MainMenuAPI.h"
#include "Random.h"
#include "LoadMonitor.h"
#include "PDA.h"
#include "GamePlayRenderPipeline.h"

class App;
class SpriteBatch;
class SpriteFont;
struct TerrainMeshMessage;

class GamePlayScreen : public IAppScreen<App> {
    friend class PdaAwesomiumAPI;
public:
    CTOR_APP_SCREEN_DECL(GamePlayScreen, App);

    virtual i32 getNextScreen() const override;
    virtual i32 getPreviousScreen() const override;

    virtual void build() override;
    virtual void destroy(const GameTime& gameTime) override;

    virtual void onEntry(const GameTime& gameTime) override;
    virtual void onExit(const GameTime& gameTime) override;

    virtual void onEvent(const SDL_Event& e) override;
    virtual void update(const GameTime& gameTime) override;
    virtual void draw(const GameTime& gameTime) override;

    // Getters
    i32 getWindowWidth() const;
    i32 getWindowHeight() const;

private:

    /// Initializes the rendering
    void initRenderPipeline();

    /// Handles updating state based on input
    void handleInput();

    /// Handles mouse down input for player
    void onMouseDown(const SDL_Event& e);

    /// Handles mouse up input for player
    void onMouseUp(const SDL_Event& e);

    /// Updates the player
    void updatePlayer();

    /// The function that runs on the update thread. It handles
    /// loading the planet in the background.
    void updateThreadFunc();

    /// Processes messages from the update->render thread
    void processMessages();

    /// Updates the dynamic clipping plane for the world camera
    void updateWorldCameraClip();

    Player* _player; ///< The current player

    PDA _pda; ///< The PDA

    bool _inFocus; ///< true when the window is in focus

    // TODO(Ben): Should they be stored here?
    //Camera _voxelCamera; ///< The camera for rendering the voxels
    //Camera _planetCamera; ///< The camera for rendering the planet

    std::thread* _updateThread; ///< The thread that updates the planet. Runs updateThreadFunc()
    volatile bool _threadRunning; ///< True when the thread should be running

    GamePlayRenderPipeline _renderPipeline; ///< This handles all rendering for the screen
};

#endif // GAMEPLAYSCREEN_H_