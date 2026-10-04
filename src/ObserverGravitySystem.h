#pragma once

#include <vector>
#include <glm/glm.hpp>

class CelestialBody;
class PhysicalObserver;

struct ObserverGravitySample
{
    glm::dvec3 Acceleration = glm::dvec3(0.0);
    int DominantBodyIndex = -1;
    double DominantAcceleration = 0.0;
    double DominantDistance = 0.0;
    bool DominantIsBlackHole = false;
    double DominantSchwarzschildRadius = 0.0;
    bool InsideEventHorizon = false;
};

class ObserverGravitySystem
{
public:
    ObserverGravitySample Sample(const glm::dvec3& position, const std::vector<CelestialBody>& bodies) const;
    void Update(PhysicalObserver& observer, const std::vector<CelestialBody>& bodies, double coordinateDeltaTime);
    const ObserverGravitySample& GetLastSample() const;

private:
    ObserverGravitySample lastSample;
    double CalculateAdaptiveStep(const PhysicalObserver& observer, const std::vector<CelestialBody>& bodies, double remainingTime) const;
};
