// 
//  PdaAwesomiumAPI.h
//  Seed Of Andromeda
//
//  Created by Ben Arnold on 26 Oct 2014
//  Copyright 2014 Regrowth Studios
//  All Rights Reserved
//  
//  This file provides the implementation for awesomium
//  interface callback API for the PDA
//

#pragma once

#ifndef PDAAWESOMIUMAPI_H_
#define PDAAWESOMIUMAPI_H_

#include "IAwesomiumAPI.h"

class IGameScreen;
class GamePlayScreen;

/// Awesomium API for PDA
class PdaAwesomiumAPI : public vui::IAwesomiumAPI<PdaAwesomiumAPI>
{
public:
    /// Initializes the API and hooks up all functions
    /// @oaram interfaceObject: The object that the API will talk to
    /// @param ownerScreen: The GamePlayScreen that owns this interface
    void init(Awesomium::JSObject* interfaceObject, IGameScreen* ownerScreen);

    // Sets the owner screen. Should be a GamePlayScreen type
    /// @param ownerScreen: The screen
    void setOwnerScreen(IGameScreen* ownerScreen);

private:

    /// Gets a list of all items
    /// @param args: Empty arguments.
    /// @return array of pairs specified as:
    /// pair<string blockName, integer num>
    Awesomium::JSValue getInventory(const Awesomium::JSArray& args);

    /// Indicates that the user has selected an inventory item
    /// @param args: Argumants. args[0] should be an integer. 0 = LMB, 1 = MMB, 2 = RMB 
    /// args[1] should be the name of the item.
    void selectItem(const Awesomium::JSArray& args);

    GamePlayScreen* _ownerScreen; ///< Handle to the main menu screen
};

#endif // PDAAWESOMIUMAPI_H_