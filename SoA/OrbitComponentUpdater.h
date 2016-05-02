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
struct NamePositionComponent;
struct SpaceBodyComponent;
struct SphericalGravityComponent;

class OrbitComponentUpdater {
public:
    void update(SpaceSystem* spaceSystem, f64 time);

    /// Updates the position based on time and parent position
    /// @param cmp: The component to update
    /// @param time: Time in seconds
    /// @param npComponent: The positional component of this component
    /// @param parentNpComponent: The parents positional component
    void updatePosition(SpaceBodyComponent& cmp, f64 time, NamePositionComponent* npComponent,
                           SpaceBodyComponent* parentOrbComponent = nullptr,
                           NamePositionComponent* parentNpComponent = nullptr);

    f64 calculateTrueAnomaly(f64 meanAnomaly, f64 e);
};

#endif // OrbitComponentUpdater_h__