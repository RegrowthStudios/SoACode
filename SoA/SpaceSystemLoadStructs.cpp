#include "stdafx.h"
#include "SpaceSystemLoadStructs.h"

KEG_ENUM_DEF(SpaceBodyGenerationType, SpaceBodyGenerationType, kt) {
    kt.addValue("planet", SpaceBodyGenerationType::PLANET);
    kt.addValue("star", SpaceBodyGenerationType::STAR);
    kt.addValue("gasGiant", SpaceBodyGenerationType::GAS_GIANT);
}

KEG_ENUM_DEF(SpaceBodyType, SpaceBodyType, kt) {
    kt.addValue("Barycenter", SpaceBodyType::BARYCENTER);
    kt.addValue("Star", SpaceBodyType::STAR);
    kt.addValue("Planet", SpaceBodyType::PLANET);
    kt.addValue("DwarfPlanet", SpaceBodyType::DWARF_PLANET);
    kt.addValue("Moon", SpaceBodyType::MOON);
    kt.addValue("DwarfMoon", SpaceBodyType::DWARF_MOON);
    kt.addValue("Asteroid", SpaceBodyType::ASTEROID);
    kt.addValue("Comet", SpaceBodyType::COMET);
}

KEG_ENUM_DEF(TrojanType, TrojanType, kt) {
    kt.addValue("L4", TrojanType::L4);
    kt.addValue("L5", TrojanType::L5);
}

KEG_TYPE_DEF_SAME_NAME(SystemBodyProperties, kt) {
    kt.addValue("type", keg::Value::custom(offsetof(SystemBodyProperties, bodyType), "SpaceBodyType", true));
    kt.addValue("trojan", keg::Value::custom(offsetof(SystemBodyProperties, trojan), "TrojanType", true));
    kt.addValue("comps", keg::Value::array(offsetof(SystemBodyProperties, comps), keg::BasicType::C_STRING));
    kt.addValue("par", keg::Value::basic(offsetof(SystemBodyProperties, name), keg::BasicType::STRING));
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, path, STRING);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, ref, STRING);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, e, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, t, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, a, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, n, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, p, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, i, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, RA, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, dec, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, dist, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, td, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, tf, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, diameter, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, density, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, mass, F64);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, aTilt, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, lNorth, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, SystemBodyProperties, rotationalPeriod, F64);
}

KEG_TYPE_DEF_SAME_NAME(AtmosphereProperties, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, AtmosphereProperties, kr, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, AtmosphereProperties, km, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, AtmosphereProperties, g, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, AtmosphereProperties, scaleDepth, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, AtmosphereProperties, waveLength, F32_V3);
}

KEG_TYPE_DEF_SAME_NAME(PlanetRingProperties, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, PlanetRingProperties, innerRadius, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, PlanetRingProperties, outerRadius, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, PlanetRingProperties, aTilt, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, PlanetRingProperties, lNorth, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, PlanetRingProperties, colorLookup, STRING);
}

KEG_TYPE_DEF_SAME_NAME(CloudsProperties, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, CloudsProperties, color, F32_V3);
    KEG_TYPE_INIT_ADD_MEMBER(kt, CloudsProperties, scale, F32_V3);
    KEG_TYPE_INIT_ADD_MEMBER(kt, CloudsProperties, density, F32);
}

KEG_TYPE_DEF_SAME_NAME(PlanetProperties, kt) {
    kt.addValue("atmosphere", keg::Value::custom(offsetof(PlanetProperties, atmosphere), "AtmosphereKegProperties", false));
    kt.addValue("clouds", keg::Value::custom(offsetof(PlanetProperties, clouds), "CloudsKegProperties", false));
}

KEG_TYPE_DEF_SAME_NAME(StarProperties, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, StarProperties, surfaceTemperature, F64);
}

KEG_TYPE_DEF_SAME_NAME(GasGiantProperties, kt) {
    KEG_TYPE_INIT_ADD_MEMBER(kt, GasGiantProperties, oblateness, F32);
    KEG_TYPE_INIT_ADD_MEMBER(kt, GasGiantProperties, colorMap, STRING);
    kt.addValue("atmosphere", keg::Value::custom(offsetof(GasGiantProperties, atmosphere), "AtmosphereKegProperties", false));
    kt.addValue("rings", keg::Value::array(offsetof(GasGiantProperties, rings), keg::Value::custom(0, "PlanetRingProperties", false)));
}
