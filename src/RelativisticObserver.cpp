#include "RelativisticObserver.h"

#include <algorithm>
#include <cmath>

#include "PhysicsConstants.h"

namespace
{
    constexpr double MaxGamma = 1.0e6;
    constexpr double MinProperAccelerationG = 0.01;
    constexpr double MaxProperAccelerationG = 10000.0;
    constexpr double MaxRapidityIncrementPerStep = 0.005;
}

void RelativisticObserver::Enter(const glm::dvec3& newPosition, const glm::dvec3&, double coordinateTime)
{
    active = true;
    thrusting = false;
    position = newPosition;
    velocity = glm::dvec3(0.0);
    properTime = coordinateTime;
    properAcceleration = PhysicsConstants::StandardGravity;
}

void RelativisticObserver::Exit()
{
    active = false;
    thrusting = false;
}

void RelativisticObserver::ProcessThrottleControl(double realDeltaTime, bool increase, bool decrease)
{
    if (!active || realDeltaTime <= 0.0 || increase == decrease)
        return;

    double factor = std::exp(throttleChangeRate * realDeltaTime);
    double minAcceleration = PhysicsConstants::StandardGravity * MinProperAccelerationG;
    double maxAcceleration = PhysicsConstants::StandardGravity * MaxProperAccelerationG;

    if (increase)
        properAcceleration = std::min(maxAcceleration, properAcceleration * factor);
    else
        properAcceleration = std::max(minAcceleration, properAcceleration / factor);
}

void RelativisticObserver::ApplyProperBoost(const glm::dvec3& localDirection, double rapidityIncrement)
{
    double directionLength = glm::length(localDirection);

    if (directionLength <= 0.0 || rapidityIncrement <= 0.0)
        return;

    glm::dvec3 direction = localDirection / directionLength;
    double relativeSpeed = PhysicsConstants::C * std::tanh(rapidityIncrement);
    glm::dvec3 relativeVelocity = direction * relativeSpeed;

    double speed = glm::length(velocity);

    if (speed <= 0.0)
    {
        velocity = relativeVelocity;
        EnforceNumericalLimit();
        return;
    }

    glm::dvec3 velocityDirection = velocity / speed;
    glm::dvec3 relativeParallel = glm::dot(relativeVelocity, velocityDirection) * velocityDirection;
    glm::dvec3 relativePerpendicular = relativeVelocity - relativeParallel;
    double gamma = GetGamma();
    double denominator = 1.0 + glm::dot(velocity, relativeVelocity) / (PhysicsConstants::C * PhysicsConstants::C);

    glm::dvec3 newParallel = (velocity + relativeParallel) / denominator;
    glm::dvec3 newPerpendicular = relativePerpendicular / (gamma * denominator);

    velocity = newParallel + newPerpendicular;
    EnforceNumericalLimit();
}

void RelativisticObserver::EnforceNumericalLimit()
{
    double speed = glm::length(velocity);

    if (speed <= 0.0)
        return;

    double betaMax = std::sqrt(1.0 - 1.0 / (MaxGamma * MaxGamma));
    double maxSpeed = PhysicsConstants::C * betaMax;

    if (speed >= maxSpeed)
        velocity *= maxSpeed / speed;
}

void RelativisticObserver::Update(double coordinateDeltaTime, bool thrust, const glm::dvec3& localThrustDirection)
{
    if (!active)
        return;

    thrusting = thrust && glm::length(localThrustDirection) > 0.0;

    if (coordinateDeltaTime <= 0.0)
        return;

    if (!thrusting)
    {
        double gamma = GetGamma();
        position += velocity * coordinateDeltaTime;
        properTime += coordinateDeltaTime / gamma;
        return;
    }

    double estimatedProperDelta = coordinateDeltaTime / GetGamma();
    double estimatedRapidityChange = properAcceleration * estimatedProperDelta / PhysicsConstants::C;
    int stepCount = std::max(1, static_cast<int>(std::ceil(std::abs(estimatedRapidityChange) / MaxRapidityIncrementPerStep)));
    stepCount = std::min(stepCount, 4096);
    double stepDelta = coordinateDeltaTime / static_cast<double>(stepCount);

    for (int i = 0; i < stepCount; i++)
    {
        glm::dvec3 oldVelocity = velocity;
        double oldGamma = GetGamma();
        double properDelta = stepDelta / oldGamma;
        double rapidityIncrement = properAcceleration * properDelta / PhysicsConstants::C;

        ApplyProperBoost(localThrustDirection, rapidityIncrement);

        double newGamma = GetGamma();
        position += 0.5 * (oldVelocity + velocity) * stepDelta;
        properTime += 0.5 * (1.0 / oldGamma + 1.0 / newGamma) * stepDelta;
    }
}

bool RelativisticObserver::IsActive() const
{
    return active;
}

bool RelativisticObserver::IsThrusting() const
{
    return thrusting;
}

bool RelativisticObserver::IsAtNumericalLimit() const
{
    return GetGamma() >= MaxGamma * (1.0 - 1.0e-9);
}

const glm::dvec3& RelativisticObserver::GetPosition() const
{
    return position;
}

const glm::dvec3& RelativisticObserver::GetVelocity() const
{
    return velocity;
}

double RelativisticObserver::GetBeta() const
{
    return GetSpeed() / PhysicsConstants::C;
}

double RelativisticObserver::GetOneMinusBeta() const
{
    double beta = GetBeta();
    double gamma = GetGamma();

    if (beta <= 0.0)
        return 1.0;

    return 1.0 / (gamma * gamma * (1.0 + beta));
}

double RelativisticObserver::GetSpeed() const
{
    return glm::length(velocity);
}

double RelativisticObserver::GetGamma() const
{
    double beta = glm::length(velocity) / PhysicsConstants::C;
    double betaSquared = beta * beta;

    if (betaSquared >= 1.0)
        return MaxGamma;

    return 1.0 / std::sqrt(1.0 - betaSquared);
}

double RelativisticObserver::GetRapidity() const
{
    return std::acosh(GetGamma());
}

double RelativisticObserver::GetTimeRate() const
{
    return 1.0 / GetGamma();
}

double RelativisticObserver::GetProperTime() const
{
    return properTime;
}

double RelativisticObserver::GetProperAcceleration() const
{
    return properAcceleration;
}

double RelativisticObserver::GetProperAccelerationG() const
{
    return properAcceleration / PhysicsConstants::StandardGravity;
}

double RelativisticObserver::GetMaxGamma() const
{
    return MaxGamma;
}
