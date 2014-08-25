#include "stdafx.h"
#include "Player.h"

#include <glm\gtx\quaternion.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include "BlockData.h"
#include "GameManager.h"
#include "InputManager.h"
#include "Inputs.h"
#include "Item.h"
#include "Options.h"
#include "Planet.h"
#include "Rendering.h"
#include "Rendering.h"
#include "Texture2d.h"
#include "utils.h"

// TODO: Get Rid Of This
using namespace glm;

#define MAX(a,b) ((a)>(b)?(a):(b))

Player *player = NULL;

Player::Player() : scannedBlock(0),
                    _mass(100),
                    headInBlock(NONE),
                    rightEquippedItem(NULL),
                    leftEquippedItem(NULL),
                    lightActive(0),
                    isClimbing(0),
                    _cameraBobTheta(0),
                    _wasSpacePressedLastFrame(0),
                    isOnPlanet(1), 
                    _maxJump(40),
                    _crouch(0),
                    _jumpForce(0.16),
                    _jumpWindup(0),
                    _jumpVal(0),
                    _isJumping(0),
                    _heightMod(0.0f),
                    currBiome(NULL),
                    currTemp(-1),
                    currHumidity(-1),
                    _moveMod(1.0),
                    canCling(0),
                    isClinging(0),
                    _rolling(0),
                    _name("Nevin"),
                    isSprinting(0),
                    vel(0),
                    _speed(0.25f),
                    _zoomLevel(0),
                    _zoomSpeed(0.04f),
                    _zoomPercent(1.0f),
                    isGrounded(0),
                    _acceleration(1.4f),
                    _maxVelocity(1.0f),
                    currCh(NULL),
                    isSwimming(0),
                    isUnderWater(0),
                    isDragBreak(0),
                    dragBlock(NULL)
{
    faceData.rotation = 0;

    faceData.face = 0;

    velocity.x = 0;
    velocity.y = 0;
    velocity.z = 0;
    

    facePosition.x = gridPosition.x = 0;
    facePosition.z = gridPosition.z = 0;

    // Initial position : on +Z

    // Projection matrix : 45