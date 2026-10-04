#pragma once

#include <glm/glm.hpp>

class RelativisticObserver
{
public:
    void Enter(const glm::dvec3& position, const glm::dvec3& direction, double coordinateTime);
    void Exit();

    void ProcessThrottleControl(double realDeltaTime, bool increase, bool decrease);
    void Update(double coordinateDeltaTime, bool thrust, const glm::dvec3& localThrustDirection);

    bool IsActive() const;
    bool IsThrusting() const;
    bool IsAtNumericalLimit() const;

    const glm::dvec3& GetPosition() const;
    const glm::dvec3& GetVelocity() const;

    double GetBeta() const;
    double GetOneMinusBeta() const;
    double GetSpeed() const;
    double GetGamma() const;
    double GetRapidity() const;
    double GetTimeRate() const;
    double GetProperTime() const;
    double GetProperAcceleration() const;
    double GetProperAccelerationG() const;
    double GetMaxGamma() const;

private:
    bool active = false;
    bool thrusting = false;

    glm::dvec3 position = glm::dvec3(0.0);
    glm::dvec3 velocity = glm::dvec3(0.0);

    double properTime = 0.0;
    double properAcceleration = 9.80665;
    double throttleChangeRate = 1.0;

    void ApplyProperBoost(const glm::dvec3& localDirection, double rapidityIncrement);
    void EnforceNumericalLimit();
};
