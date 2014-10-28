#pragma once
#ifndef _Texture2d_h_
#define _Texture2d_h_

#include "stdafx.h"
#include <SDL/SDL.h>

#include "Constants.h"
#include "TextureCache.h"

vg::Texture getTexture(string source, struct Animation **anim = NULL);

struct BlockPack
{
    void initialize(vg::Texture texInfo);
    vg::Texture textureInfo;
    vector <GLubyte[256][3]> avgColors;
};

extern BlockPack blockPack; //TODO: Not global

extern vg::Texture markerTexture;
extern vg::Texture terrainTexture;
extern vg::Texture logoTexture;
extern vg::Texture sunTexture;
extern vg::Texture waterNormalTexture;
extern vg::Texture cloudTexture1;
extern vg::Texture WaterTexture;
extern vg::Texture normalLeavesTexture, pineLeavesTexture, mushroomCapTexture, treeTrunkTexture1;
extern vg::Texture ballMaskTexture;
extern vg::Texture starboxTextures[6];
extern vg::Texture BlankTextureID;
extern vg::Texture explosionTexture;
extern vg::Texture fireTexture;
extern vg::Texture waterNoiseTexture;
extern vg::Texture crosshairTexture;

#endif // _Texture2d_h_