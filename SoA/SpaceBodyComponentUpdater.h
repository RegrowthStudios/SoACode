///
/// OrbitComponentUpdater.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 8 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Updates OrbitComponents
///

#pragma once

#ifndef OrbitComponentUpdater_h__
#define OrbitComponentUpdater_h__

class SpaceSystem;
struct SpaceBodyComponent;

class SpaceBodyComponentUpdater {
public:
    void update(SpaceSystem* spaceSystem, f64 time);

    /// Updates the position based on time and parent position
    /// @param cmp: The component to update
    /// @param time: Time in seconds
    /// @param npComponent: The positional component of this component
    /// @param parentNpComponent: The parents positional component
    void updatePosition(SpaceBodyComponent& cmp, OPT SpaceBodyComponent* parentCmp, f64 time);
    void updateAxisRotation(SpaceBodyComponent& cmp, f64 time);

    void getPositionAndVelocity(const SpaceBodyComponent& cmp, OPT const SpaceBodyComponent* parentCmp, f64 time,
                                OUT f64v3& outPosition, OUT f64v3& outVelocity, OUT f64& outMeanAnomaly);

    f64 calculateTrueAnomaly(f64 meanAnomaly, f64 e);
};

#endif // OrbitComponentUpdater_h__