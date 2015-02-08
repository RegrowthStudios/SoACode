#include "stdafx.h"
#include "PlanetData.h"

KEG_TYPE_INIT_BEGIN_DEF_VAR(LiquidColorKegProperties)
KEG_TYPE_INIT_ADD_MEMBER(LiquidColorKegProperties, STRING, colorPath);
KEG_TYPE_INIT_ADD_MEMBER(LiquidColorKegProperties, STRING, texturePath);
KEG_TYPE_INIT_ADD_MEMBER(LiquidColorKegProperties, UI8_V3, tint);
KEG_TYPE_INIT_ADD_MEMBER(LiquidColorKegProperties, F32, depthScale);
KEG_TYPE_INIT_ADD_MEMBER(LiquidColorKegProperties, F32, freezeTemp);
KEG_TYPE_INIT_END

KEG_TYPE_INIT_BEGIN_DEF_VAR(TerrainColorKegProperties)
KEG_TYPE_INIT_ADD_MEMBER(TerrainColorKegProperties, STRING, colorPath);
KEG_TYPE_INIT_ADD_MEMBER(TerrainColorKegProperties, STRING, texturePath);
KEG_TYPE_INIT_ADD_MEMBER(TerrainColorKegProperties, UI8_V3, tint);
KEG_TYPE_INIT_END

KEG_ENUM_INIT_BEGIN(TerrainFunction, TerrainFunction, e);
e->addValue("noise", TerrainFunction::RIDGED_NOISE);
e->addValue("ridgedNoise", TerrainFunction::RIDGED_NOISE);
KEG_ENUM_INIT_END

KEG_TYPE_INIT_BEGIN_DEF_VAR(TerrainFuncKegProperties)
KEG_TYPE_INIT_ADD_MEMBER(TerrainFuncKegProperties, I32, octaves);
KEG_TYPE_INIT_ADD_MEMBER(TerrainFuncKegProperties, F32, persistence);
KEG_TYPE_INIT_ADD_MEMBER(TerrainFuncKegProperties, F32, frequency);
KEG_TYPE_INIT_ADD_MEMBER(TerrainFuncKegProperties, F32, low);
KEG_TYPE_INIT_ADD_MEMBER(TerrainFuncKegProperties, F32, high);
KEG_TYPE_INIT_END