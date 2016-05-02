#include "stdafx.h"
#include "SpaceBodyComponentUpdater.h"
#include "SpaceSystem.h"

#include "Constants.h"
#include "soaUtils.h"

void SpaceBodyComponentUpdater::update(SpaceSystem* spaceSystem, f64 time) {
    for (auto& it : spaceSystem->spaceBody) {
        auto& cmp = it.second;

        // Update axis rotation
        if (cmp.axisPeriod) {
            updateAxisRotation(cmp, time);
        }

        // Update position
        if (cmp.parent) {
            SpaceBodyComponent* parentCmp = &spaceSystem->spaceBody.get(cmp.parent);
            updatePosition(cmp, parentCmp, time);
        } else {
            updatePosition(cmp, nullptr, time);
        }
        
    }
}

void SpaceBodyComponentUpdater::updatePosition(SpaceBodyComponent& cmp, OPT SpaceBodyComponent* parentCmp, f64 time) {
    assert(cmp.major != 0.0);
    // TODO(Ben): There is potential for more cacheing in here
    /// Calculates position as a function of time
    /// http://en.wikipedia.org/wiki/Kepler%27s_laws_of_planetary_motion#Position_as_a_function_of_time

    // 1. Calculate the mean anomaly
    f64 meanAnomaly = (M_2_PI / cmp.t) * time + cmp.startMeanAnomaly;
    cmp.currentMeanAnomaly = (f32)meanAnomaly;

    f64 v = calculateTrueAnomaly(meanAnomaly, cmp.e);
  
    // Calculate radius
    // http://www.stargazing.net/kepler/ellipse.html
    f64 r = cmp.major * (1.0 - cmp.e * cmp.e) / (1.0 + cmp.e * cos(v));
    
    f64 w = cmp.p - cmp.n; ///< Argument of periapsis

    // Calculate position
    f64v3 position;
    f64 cosv = cos(v + cmp.p - cmp.n);
    f64 sinv = sin(v + cmp.p - cmp.n);
    f64 coso = cos(cmp.n);
    f64 sino = sin(cmp.n);
    f64 cosi = cos(cmp.i);
    f64 sini = sin(cmp.i);
    position.x = r * (coso * cosv - sino * sinv * cosi);
    position.y = r * (sinv * sini);
    position.z = r * (sino * cosv + coso * sinv * cosi);

    // Calculate velocity
    // TODO(Ben): Cache g INSTEAD of parentMass
    f64 g = sqrt(M_G * KM_PER_M * cmp.parentMass * (2.0 / r - 1.0 / cmp.major)) * KM_PER_M;
    f64 sinwv = sin(w + v);
    cmp.relativeVelocity.x = -g * sinwv * cosi;
    cmp.relativeVelocity.y = g * sinwv * sini;
    cmp.relativeVelocity.z = g * cos(w + v);

    // If this planet has a parent, make it parent relative
    if (parentCmp) {
        cmp.velocity = parentCmp->velocity + cmp.relativeVelocity;
        cmp.position = position + parentCmp->position;
    } else {
        cmp.velocity = cmp.relativeVelocity;
        cmp.position = position;
    }
}

f64 SpaceBodyComponentUpdater::calculateTrueAnomaly(f64 meanAnomaly, f64 e) {
    // 2. Solve Kepler's equation to compute eccentric anomaly 
    // using Newton's method
    // http://www.jgiesen.de/kepler/kepler.html
#define ITERATIONS 3
    f64 E; ///< Eccentric Anomaly
    f64 F;
    E = meanAnomaly;
    F = E - e * sin(meanAnomaly) - meanAnomaly;
    for (int n = 0; n < ITERATIONS; n++) {
        E = E -
            F / (1.0 - e * cos(E));
        F = E -
            e * sin(E) - meanAnomaly;
    }
    // 3. Calculate true anomaly
    return atan2(sqrt(1.0 - e * e) * sin(E), cos(E) - e);
}

void SpaceBodyComponentUpdater::updateAxisRotation(SpaceBodyComponent& cmp, f64 time) {
    // Calculate rotation  
    cmp.currentRotation = (time / cmp.axisPeriod) * 2.0 * M_PI;

    // Calculate the axis rotation quat
    f64v3 eulerAngles(0, -cmp.currentRotation, 0);
    f64q rotationQuat = f64q(eulerAngles);

    // Calculate total orientation
    cmp.currentOrientation = cmp.axisOrientation * rotationQuat;
    cmp.invCurrentOrientation = vmath::inverse(cmp.currentOrientation);  
}
