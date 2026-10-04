#include "ObserverGravitySystem.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "CelestialBody.h"
#include "PhysicalObserver.h"
#include "PhysicsConstants.h"

namespace
{
    constexpr double SpatialFraction = 0.05;
    constexpr double GravityFraction = 0.025;
    constexpr double MinAdaptiveStep = 1.0e-5;
    constexpr int MaxAdaptiveSteps = 4096;
}

ObserverGravitySample ObserverGravitySystem::Sample(const glm::dvec3& position, const std::vector<CelestialBody>& bodies) const
{
    ObserverGravitySample sample;

    for (std::size_t i = 0; i < bodies.size(); i++)
    {
        const CelestialBody& body = bodies[i];

        if (body.Mass <= 0.0)
            continue;

        glm::dvec3 delta = body.Position - position;
        double distanceSquared = glm::dot(delta, delta);

        if (distanceSquared <= 0.0)
            continue;

        double distance = std::sqrt(distanceSquared);
        glm::dvec3 direction = delta / distance;
        double accelerationMagnitude;

        if (body.Type == CelestialBodyType::BlackHole)
            accelerationMagnitude = PhysicsConstants::G * body.Mass / distanceSquared;
        else if (body.Radius > 0.0 && distance < body.Radius)
            accelerationMagnitude = PhysicsConstants::G * body.Mass * distance / (body.Radius * body.Radius * body.Radius);
        else
            accelerationMagnitude = PhysicsConstants::G * body.Mass / distanceSquared;

        sample.Acceleration += direction * accelerationMagnitude;

        if (accelerationMagnitude > sample.DominantAcceleration)
        {
            sample.DominantAcceleration = accelerationMagnitude;
            sample.DominantBodyIndex = static_cast<int>(i);
            sample.DominantDistance = distance;
            sample.DominantIsBlackHole = body.Type == CelestialBodyType::BlackHole;
            sample.DominantSchwarzschildRadius = sample.DominantIsBlackHole ? PhysicsConstants::SchwarzschildRadius(body.Mass) : 0.0;
            sample.InsideEventHorizon = sample.DominantIsBlackHole && distance <= sample.DominantSchwarzschildRadius;
        }
    }

    return sample;
}

double ObserverGravitySystem::CalculateAdaptiveStep(const PhysicalObserver& observer, const std::vector<CelestialBody>& bodies, double remainingTime) const
{
    double bestStep = remainingTime;
    const glm::dvec3& position = observer.GetPosition();
    const glm::dvec3& velocity = observer.GetVelocity();

    for (const CelestialBody& body : bodies)
    {
        if (body.Mass <= 0.0)
            continue;

        glm::dvec3 relativePosition = body.Position - position;
        double distance = glm::length(relativePosition);

        if (distance <= 0.0)
            return std::min(remainingTime, MinAdaptiveStep);

        glm::dvec3 relativeVelocity = body.Velocity - velocity;
        double relativeSpeed = std::max(glm::length(relativeVelocity), 1.0);
        double crossingTime = SpatialFraction * distance / relativeSpeed;
        double gravityTime = GravityFraction * std::sqrt(distance * distance * distance / (PhysicsConstants::G * body.Mass));
        double localStep = std::min(crossingTime, gravityTime);

        if (body.Type == CelestialBodyType::BlackHole)
        {
            double schwarzschildRadius = PhysicsConstants::SchwarzschildRadius(body.Mass);

            if (schwarzschildRadius > 0.0 && distance < 20.0 * schwarzschildRadius)
            {
                double horizonScale = std::max(std::abs(distance - schwarzschildRadius), 0.001 * schwarzschildRadius);
                double horizonStep = 0.05 * horizonScale / std::max(relativeSpeed, PhysicsConstants::C * 0.001);
                localStep = std::min(localStep, horizonStep);
            }
        }

        if (std::isfinite(localStep) && localStep > 0.0)
            bestStep = std::min(bestStep, localStep);
    }

    return std::clamp(bestStep, std::min(MinAdaptiveStep, remainingTime), remainingTime);
}

void ObserverGravitySystem::Update(PhysicalObserver& observer, const std::vector<CelestialBody>& bodies, double coordinateDeltaTime)
{
    if (!observer.IsActive())
        return;

    if (coordinateDeltaTime <= 0.0)
    {
        lastSample = Sample(observer.GetPosition(), bodies);
        return;
    }

    if (!observer.IsMotionActive())
    {
        observer.IntegrateStep(coordinateDeltaTime, glm::dvec3(0.0));
        lastSample = Sample(observer.GetPosition(), bodies);
        return;
    }

    double remainingTime = coordinateDeltaTime;
    int stepCount = 0;

    while (remainingTime > 0.0 && stepCount < MaxAdaptiveSteps)
    {
        ObserverGravitySample sample = Sample(observer.GetPosition(), bodies);
        double stepDelta = CalculateAdaptiveStep(observer, bodies, remainingTime);
        observer.IntegrateStep(stepDelta, sample.Acceleration);
        remainingTime -= stepDelta;
        stepCount++;
    }

    if (remainingTime > 0.0)
        observer.IntegrateStep(remainingTime, glm::dvec3(0.0));

    lastSample = Sample(observer.GetPosition(), bodies);
}

const ObserverGravitySample& ObserverGravitySystem::GetLastSample() const
{
    return lastSample;
}
