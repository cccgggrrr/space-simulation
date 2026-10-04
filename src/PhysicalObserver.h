#pragma once

#include <glm/glm.hpp>

class PhysicalObserver
{
public:
    void Enter(const glm::dvec3& position, double coordinateTime);
    void Exit();
    void Hold();
    void Translate(const glm::dvec3& delta);
    void Launch(const glm::dvec3& direction);
    void Steer(const glm::dvec3& direction);

    void ProcessSelectedSpeedControl(double realDeltaTime, bool increase, bool decrease);
    void IntegrateStep(double coordinateDeltaTime, double observerDeltaTime, const glm::dvec3& gravityAcceleration);
    void IntegrateStep(double coordinateDeltaTime, const glm::dvec3& gravityAcceleration);

    bool IsActive() const;
    bool IsMotionActive() const;
    bool IsAtNumericalLimit() const;

    const glm::dvec3& GetPosition() const;
    const glm::dvec3& GetVelocity() const;

    double GetMass() const;
    double GetSelectedRapidity() const;
    double GetSelectedBeta() const;
    double GetSelectedOneMinusBeta() const;
    double GetSelectedLog10OneMinusBeta() const;
    double GetSelectedLog10Gamma() const;
    double GetSelectedSpeed() const;
    double GetBeta() const;
    double GetOneMinusBeta() const;
    double GetLog10OneMinusBeta() const;
    double GetSpeed() const;
    double GetGamma() const;
    double GetLog10Gamma() const;
    double GetRapidity() const;
    double GetProperVelocityMagnitude() const;
    double GetLog10ProperVelocityC() const;
    double GetTimeRate() const;
    double GetProperTime() const;
    double GetMaxGamma() const;

private:
    bool active = false;
    bool motionActive = false;

    glm::dvec3 position = glm::dvec3(0.0);
    glm::dvec3 velocity = glm::dvec3(0.0);
    glm::dvec3 travelDirection = glm::dvec3(0.0, 0.0, -1.0);

    double mass = 20000.0;
    double selectedRapidity = 0.0;
    double currentRapidity = 0.0;
    double properTime = 0.0;
    double selectedSpeedControlRate = 0.85;

    void ApplySelectedSpeed();
    void SyncVelocityFromRapidity();
    glm::dvec3 ProperVelocityFromRapidity() const;
};
