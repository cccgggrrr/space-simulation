#pragma once

#include <vector>
#include <glm/glm.hpp>

class CelestialBody;

class GravitySystem
{
public:
    void Update(std::vector<CelestialBody>& bodies, double deltaTime, double maxStep = 300.0, int maxSubsteps = 1024) const;

private:
    void CalculateAccelerations(const std::vector<CelestialBody>& bodies, std::vector<glm::dvec3>& accelerations) const;
    void Step(std::vector<CelestialBody>& bodies, double deltaTime, std::vector<glm::dvec3>& oldAccelerations, std::vector<glm::dvec3>& newAccelerations) const;
    double EstimateStableStep(const std::vector<CelestialBody>& bodies, double requestedMaxStep) const;
};
