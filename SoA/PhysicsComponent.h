///
/// PhysicsComponent.h
/// Seed of Andromeda
///
/// Created by Benjamin Arnold on 10 Jan 2015
/// Copyright 2014 Regrowth Studios
/// All Rights Reserved
///
/// Summary:
/// Physics component for velocity and momentum
///

#pragma once

#ifndef PhysicsComponent_h__
#define PhysicsComponent_h__

class PhysicsComponent {
public:

    void init(f32 massKg, const f64v3& vel = f64v3(0.0)) {
        mass = massKg;
        velocity = vel;
    }

    const f64v3 computeMomentum() const { return ((f64)mass) * velocity; }

    f64v3 velocity = f64v3(0.0);
    f32 mass;
};

#endif // PhysicsComponent_h__
