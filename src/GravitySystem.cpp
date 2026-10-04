#include "GravitySystem.h"

#include <algorithm>
#include <cmath>

#include "CelestialBody.h"
#include "PhysicsConstants.h"

void GravitySystem::CalculateAccelerations(const std::vector<CelestialBody>& bodies, std::vector<glm::dvec3>& accelerations) const
{
    accelerations.assign(bodies.size(), glm::dvec3(0.0));

    for (std::size_t i = 0; i < bodies.size(); i++)
    {
        for (std::size_t j = i + 1; j < bodies.size(); j++)
        {
            glm::dvec3 delta = bodies[j].Position - bodies[i].Position;
            double distanceSquared = glm::dot(delta, delta);

            if (distanceSquared <= 0.0)
                continue;

            double inverseDistance = 1.0 / std::sqrt(distanceSquared);
            double inverseDistanceCubed = inverseDistance * inverseDistance * inverseDistance;

            accelerations[i] += PhysicsConstants::G * bodies[j].Mass * delta * inverseDistanceCubed;
            accelerations[j] -= PhysicsConstants::G * bodies[i].Mass * delta * inverseDistanceCubed;
        }
    }
}

double GravitySystem::EstimateStableStep(const std::vector<CelestialBody>& bodies, double requestedMaxStep) const
{
    double stableStep = std::max(requestedMaxStep, 1.0e-6);

    for (std::size_t i = 0; i < bodies.size(); i++)
    {
        for (std::size_t j = i + 1; j < bodies.size(); j++)
        {
            glm::dvec3 delta = bodies[j].Position - bodies[i].Position;
            double distance = glm::length(delta);
            double totalMass = bodies[i].Mass + bodies[j].Mass;

            if (distance <= 0.0 || totalMass <= 0.0)
                continue;

            double dynamicalTime = std::sqrt(distance * distance * distance / (PhysicsConstants::G * totalMass));
            stableStep = std::min(stableStep, std::max(dynamicalTime * 0.0025, 1.0e-6));
        }
    }

    return stableStep;
}

void GravitySystem::Step(std::vector<CelestialBody>& bodies, double deltaTime, std::vector<glm::dvec3>& oldAccelerations, std::vector<glm::dvec3>& newAccelerations) const
{
    CalculateAccelerations(bodies, oldAccelerations);
    double halfDtSquared = 0.5 * deltaTime * deltaTime;

    for (std::size_t i = 0; i < bodies.size(); i++)
        bodies[i].Position += bodies[i].Velocity * deltaTime + oldAccelerations[i] * halfDtSquared;

    CalculateAccelerations(bodies, newAccelerations);

    for (std::size_t i = 0; i < bodies.size(); i++)
        bodies[i].Velocity += 0.5 * (oldAccelerations[i] + newAccelerations[i]) * deltaTime;
}

void GravitySystem::Update(std::vector<CelestialBody>& bodies, double deltaTime, double maxStep, int maxSubsteps) const
{
    if (deltaTime == 0.0 || bodies.empty())
        return;

    double stableStep = EstimateStableStep(bodies, maxStep);
    long double requestedSteps = std::ceil(std::abs(static_cast<long double>(deltaTime)) / stableStep);
    int stepCount = static_cast<int>(std::clamp<long double>(requestedSteps, 1.0L, static_cast<long double>(std::max(maxSubsteps, 1))));
    double stepDelta = deltaTime / static_cast<double>(stepCount);

    std::vector<glm::dvec3> oldAccelerations(bodies.size());
    std::vector<glm::dvec3> newAccelerations(bodies.size());

    for (int i = 0; i < stepCount; i++)
        Step(bodies, stepDelta, oldAccelerations, newAccelerations);
}
