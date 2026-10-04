#pragma once

#include <algorithm>
#include <cmath>
#include <limits>
#include <glm/glm.hpp>
#include "PhysicsConstants.h"

class SpaceRocket
{
public:
    void Enter(const glm::dvec3& newPosition, double coordinateTime)
    {
        active = true;
        thrusting = false;
        position = newPosition;
        velocity = glm::dvec3(0.0);
        rapidityVector = glm::dvec3(0.0);
        coordinateAcceleration = 0.0;
        properTime = coordinateTime;
    }

    void Exit()
    {
        active = false;
        thrusting = false;
        coordinateAcceleration = 0.0;
    }

    void ResetVelocity()
    {
        velocity = glm::dvec3(0.0);
        rapidityVector = glm::dvec3(0.0);
        coordinateAcceleration = 0.0;
        thrusting = false;
    }

    void Update(double rocketDeltaTime, double coordinateDeltaTime, const glm::dvec3& thrustDirection)
    {
        if (!active)
        {
            thrusting = false;
            coordinateAcceleration = 0.0;
            return;
        }

        glm::dvec3 oldVelocity = velocity;
        glm::dvec3 oldProperVelocity = ProperVelocityVector();
        double directionLength = glm::length(thrustDirection);
        thrusting = rocketDeltaTime > 0.0 && directionLength > 0.0 && engineThrust > 0.0;

        if (thrusting)
        {
            glm::dvec3 direction = thrustDirection / directionLength;
            double rapidityDelta = GetRestAcceleration() * rocketDeltaTime / PhysicsConstants::C;
            rapidityVector += direction * rapidityDelta;
            SyncVelocityFromRapidity();
        }

        glm::dvec3 newProperVelocity = ProperVelocityVector();

        if (rocketDeltaTime > 0.0)
        {
            glm::dvec3 averageProperVelocity = 0.5 * (oldProperVelocity + newProperVelocity);

            if (std::isfinite(averageProperVelocity.x) && std::isfinite(averageProperVelocity.y) && std::isfinite(averageProperVelocity.z))
                position += averageProperVelocity * rocketDeltaTime;

            coordinateAcceleration = glm::length(velocity - oldVelocity) / rocketDeltaTime;
            properTime += rocketDeltaTime;
        }
        else
            coordinateAcceleration = 0.0;

        (void)coordinateDeltaTime;
    }

    void SetMass(double value) { mass = std::clamp(value, 1.0, 1.0e12); }
    void SetEngineThrust(double value) { engineThrust = std::clamp(value, 0.0, 1.0e18); }

    bool IsActive() const { return active; }
    bool IsThrusting() const { return thrusting; }
    bool IsAtNumericalLimit() const { return !std::isfinite(GetRapidity()); }

    const glm::dvec3& GetPosition() const { return position; }
    const glm::dvec3& GetVelocity() const { return velocity; }

    double GetMass() const { return mass; }
    double GetEngineThrust() const { return engineThrust; }
    double GetRestAcceleration() const { return engineThrust / mass; }
    double GetRestAccelerationG() const { return GetRestAcceleration() / PhysicsConstants::StandardGravity; }
    double GetCoordinateAcceleration() const { return coordinateAcceleration; }
    double GetCoordinateAccelerationG() const { return coordinateAcceleration / PhysicsConstants::StandardGravity; }

    double GetMomentumMagnitude() const
    {
        double logMomentum = GetLogMomentumMagnitude();
        return ExpIfRepresentable(logMomentum);
    }

    double GetLog10MomentumMagnitude() const
    {
        double logMomentum = GetLogMomentumMagnitude();
        return std::isfinite(logMomentum) ? logMomentum / Ln10 : -std::numeric_limits<double>::infinity();
    }

    double GetBeta() const { return BetaFromRapidity(GetRapidity()); }
    double GetOneMinusBeta() const { return ExpIfRepresentable(LogOneMinusBetaFromRapidity(GetRapidity())); }
    double GetLog10OneMinusBeta() const { return LogOneMinusBetaFromRapidity(GetRapidity()) / Ln10; }
    double GetSpeed() const { return PhysicsConstants::C * GetBeta(); }
    double GetSpeedGapMetersPerSecond() const { return ExpIfRepresentable(std::log(PhysicsConstants::C) + LogOneMinusBetaFromRapidity(GetRapidity())); }
    double GetLog10SpeedGapMetersPerSecond() const { return std::log10(PhysicsConstants::C) + GetLog10OneMinusBeta(); }
    double GetGamma() const { return ExpIfRepresentable(LogGammaFromRapidity(GetRapidity())); }
    double GetLog10Gamma() const { return LogGammaFromRapidity(GetRapidity()) / Ln10; }
    double GetRapidity() const { return glm::length(rapidityVector); }
    double GetProperVelocityMagnitude() const
    {
        double rapidity = GetRapidity();
        if (rapidity <= 0.0)
            return 0.0;
        return ExpIfRepresentable(std::log(PhysicsConstants::C) + LogSinhFromRapidity(rapidity));
    }
    double GetLog10ProperVelocityC() const
    {
        double rapidity = GetRapidity();
        if (rapidity <= 0.0)
            return -std::numeric_limits<double>::infinity();
        return LogSinhFromRapidity(rapidity) / Ln10;
    }
    double GetTimeRate() const { return ExpIfRepresentable(-LogGammaFromRapidity(GetRapidity())); }
    double GetProperTime() const { return properTime; }
    double GetMaxGamma() const { return std::numeric_limits<double>::infinity(); }
    unsigned int GetLastSubstepCount() const { return 1; }
    unsigned int GetMaxSubstepCount() const { return 1; }

private:
    static constexpr double Ln2 = 0.69314718055994530942;
    static constexpr double Ln10 = 2.30258509299404568402;
    bool active = false;
    bool thrusting = false;
    glm::dvec3 position = glm::dvec3(0.0);
    glm::dvec3 velocity = glm::dvec3(0.0);
    glm::dvec3 rapidityVector = glm::dvec3(0.0);
    double mass = 20000.0;
    double engineThrust = 2.0e8;
    double coordinateAcceleration = 0.0;
    double properTime = 0.0;

    static double ExpIfRepresentable(double logValue)
    {
        static const double logMax = std::log(std::numeric_limits<double>::max());
        static const double logMin = std::log(std::numeric_limits<double>::denorm_min());

        if (logValue > logMax)
            return std::numeric_limits<double>::infinity();
        if (logValue < logMin)
            return 0.0;

        return std::exp(logValue);
    }

    static double LogGammaFromRapidity(double rapidity)
    {
        double eta = std::abs(rapidity);

        if (eta < 20.0)
            return std::log(std::cosh(eta));

        return eta - Ln2 + std::log1p(std::exp(-2.0 * eta));
    }

    static double LogOneMinusBetaFromRapidity(double rapidity)
    {
        double eta = std::max(0.0, rapidity);

        if (eta <= 0.0)
            return 0.0;

        return Ln2 - 2.0 * eta - std::log1p(std::exp(-2.0 * eta));
    }

    static double LogSinhFromRapidity(double rapidity)
    {
        double eta = std::abs(rapidity);

        if (eta <= 0.0)
            return -std::numeric_limits<double>::infinity();
        if (eta < 20.0)
            return std::log(std::sinh(eta));

        return eta - Ln2 + std::log1p(-std::exp(-2.0 * eta));
    }

    static double BetaFromRapidity(double rapidity)
    {
        if (rapidity <= 0.0)
            return 0.0;

        double logGap = LogOneMinusBetaFromRapidity(rapidity);

        if (logGap > std::log(std::numeric_limits<double>::epsilon()))
            return 1.0 - std::exp(logGap);

        return std::nextafter(1.0, 0.0);
    }

    double GetLogMomentumMagnitude() const
    {
        double rapidity = GetRapidity();

        if (rapidity <= 0.0)
            return -std::numeric_limits<double>::infinity();

        return std::log(mass * PhysicsConstants::C) + LogSinhFromRapidity(rapidity);
    }

    glm::dvec3 ProperVelocityVector() const
    {
        double rapidity = GetRapidity();

        if (rapidity <= 0.0)
            return glm::dvec3(0.0);

        double properSpeed = ExpIfRepresentable(std::log(PhysicsConstants::C) + LogSinhFromRapidity(rapidity));

        if (!std::isfinite(properSpeed))
            return rapidityVector / rapidity * std::numeric_limits<double>::infinity();

        return rapidityVector / rapidity * properSpeed;
    }

    void SyncVelocityFromRapidity()
    {
        double rapidity = GetRapidity();

        if (rapidity <= 0.0)
        {
            velocity = glm::dvec3(0.0);
            return;
        }

        velocity = rapidityVector / rapidity * (PhysicsConstants::C * BetaFromRapidity(rapidity));
    }
};
