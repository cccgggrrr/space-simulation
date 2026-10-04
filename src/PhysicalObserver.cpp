#include "PhysicalObserver.h"

#include <algorithm>
#include <cmath>
#include <limits>

#include "PhysicsConstants.h"

namespace
{
    constexpr double Ln2 = 0.69314718055994530942;
    constexpr double Ln10 = 2.30258509299404568402;
    constexpr double MaxStoredRapidity = 1.0e300;

    double LogGammaFromRapidity(double rapidity)
    {
        double eta = std::abs(rapidity);

        if (eta < 20.0)
            return std::log(std::cosh(eta));

        return eta - Ln2 + std::log1p(std::exp(-2.0 * eta));
    }

    double LogOneMinusBetaFromRapidity(double rapidity)
    {
        double eta = std::max(0.0, rapidity);

        if (eta <= 0.0)
            return 0.0;

        return Ln2 - 2.0 * eta - std::log1p(std::exp(-2.0 * eta));
    }

    double LogSinhFromRapidity(double rapidity)
    {
        double eta = std::abs(rapidity);

        if (eta <= 0.0)
            return -std::numeric_limits<double>::infinity();
        if (eta < 20.0)
            return std::log(std::sinh(eta));

        return eta - Ln2 + std::log1p(-std::exp(-2.0 * eta));
    }

    double BetaFromRapidity(double rapidity)
    {
        if (rapidity <= 0.0)
            return 0.0;

        double logGap = LogOneMinusBetaFromRapidity(rapidity);

        if (logGap > std::log(std::numeric_limits<double>::epsilon()))
            return 1.0 - std::exp(logGap);

        return std::nextafter(1.0, 0.0);
    }

    double ExpIfRepresentable(double logValue)
    {
        static const double logMax = std::log(std::numeric_limits<double>::max());
        static const double logMin = std::log(std::numeric_limits<double>::denorm_min());

        if (logValue > logMax)
            return std::numeric_limits<double>::infinity();
        if (logValue < logMin)
            return 0.0;

        return std::exp(logValue);
    }
}

void PhysicalObserver::Enter(const glm::dvec3& newPosition, double coordinateTime)
{
    active = true;
    motionActive = false;
    position = newPosition;
    velocity = glm::dvec3(0.0);
    travelDirection = glm::dvec3(0.0, 0.0, -1.0);
    selectedRapidity = 0.0;
    currentRapidity = 0.0;
    properTime = coordinateTime;
}

void PhysicalObserver::Exit()
{
    active = false;
    motionActive = false;
}

void PhysicalObserver::Hold()
{
    motionActive = false;
    velocity = glm::dvec3(0.0);
    currentRapidity = 0.0;
}

void PhysicalObserver::Translate(const glm::dvec3& delta)
{
    if (active)
        position += delta;
}

void PhysicalObserver::Launch(const glm::dvec3& direction)
{
    if (!active)
        return;

    double directionLength = glm::length(direction);

    if (directionLength <= 0.0)
        return;

    travelDirection = direction / directionLength;
    motionActive = true;
    ApplySelectedSpeed();
}

void PhysicalObserver::Steer(const glm::dvec3& direction)
{
    if (!active || !motionActive)
        return;

    double directionLength = glm::length(direction);

    if (directionLength <= 0.0)
        return;

    travelDirection = direction / directionLength;
    ApplySelectedSpeed();
}

void PhysicalObserver::ProcessSelectedSpeedControl(double realDeltaTime, bool increase, bool decrease)
{
    if (!active || realDeltaTime <= 0.0 || increase == decrease)
        return;

    double deltaRapidity = selectedSpeedControlRate * realDeltaTime;

    if (increase)
        selectedRapidity = std::min(MaxStoredRapidity, selectedRapidity + deltaRapidity);
    else
        selectedRapidity = std::max(0.0, selectedRapidity - deltaRapidity);

    if (selectedRapidity < 1.0e-12)
        selectedRapidity = 0.0;

    if (motionActive)
        ApplySelectedSpeed();
}

void PhysicalObserver::ApplySelectedSpeed()
{
    if (!active || !motionActive)
        return;

    double directionLength = glm::length(travelDirection);

    if (directionLength <= 0.0)
        return;

    travelDirection /= directionLength;
    currentRapidity = selectedRapidity;
    SyncVelocityFromRapidity();
}

void PhysicalObserver::IntegrateStep(double coordinateDeltaTime, double observerDeltaTime, const glm::dvec3& gravityAcceleration)
{
    if (!active)
        return;

    if (observerDeltaTime > 0.0)
        properTime += observerDeltaTime;

    if (!motionActive || observerDeltaTime <= 0.0)
        return;

    glm::dvec3 properVelocity = ProperVelocityFromRapidity();

    if (std::isfinite(properVelocity.x) && std::isfinite(properVelocity.y) && std::isfinite(properVelocity.z))
        position += properVelocity * observerDeltaTime;

    (void)coordinateDeltaTime;
    (void)gravityAcceleration;
}

void PhysicalObserver::IntegrateStep(double coordinateDeltaTime, const glm::dvec3& gravityAcceleration)
{
    IntegrateStep(coordinateDeltaTime, 0.0, gravityAcceleration);
}

void PhysicalObserver::SyncVelocityFromRapidity()
{
    if (!motionActive || currentRapidity <= 0.0)
    {
        velocity = glm::dvec3(0.0);
        return;
    }

    velocity = travelDirection * (PhysicsConstants::C * BetaFromRapidity(currentRapidity));
}

glm::dvec3 PhysicalObserver::ProperVelocityFromRapidity() const
{
    if (!motionActive || currentRapidity <= 0.0)
        return glm::dvec3(0.0);

    double logProperSpeed = std::log(PhysicsConstants::C) + LogSinhFromRapidity(currentRapidity);
    double properSpeed = ExpIfRepresentable(logProperSpeed);

    if (!std::isfinite(properSpeed))
        return travelDirection * std::numeric_limits<double>::infinity();

    return travelDirection * properSpeed;
}

bool PhysicalObserver::IsActive() const { return active; }
bool PhysicalObserver::IsMotionActive() const { return motionActive; }
bool PhysicalObserver::IsAtNumericalLimit() const { return selectedRapidity >= MaxStoredRapidity; }
const glm::dvec3& PhysicalObserver::GetPosition() const { return position; }
const glm::dvec3& PhysicalObserver::GetVelocity() const { return velocity; }
double PhysicalObserver::GetMass() const { return mass; }
double PhysicalObserver::GetSelectedRapidity() const { return selectedRapidity; }
double PhysicalObserver::GetSelectedBeta() const { return BetaFromRapidity(selectedRapidity); }
double PhysicalObserver::GetSelectedOneMinusBeta() const { return ExpIfRepresentable(LogOneMinusBetaFromRapidity(selectedRapidity)); }
double PhysicalObserver::GetSelectedLog10OneMinusBeta() const { return LogOneMinusBetaFromRapidity(selectedRapidity) / Ln10; }
double PhysicalObserver::GetSelectedLog10Gamma() const { return LogGammaFromRapidity(selectedRapidity) / Ln10; }
double PhysicalObserver::GetSelectedSpeed() const { return PhysicsConstants::C * GetSelectedBeta(); }
double PhysicalObserver::GetBeta() const { return BetaFromRapidity(currentRapidity); }
double PhysicalObserver::GetOneMinusBeta() const { return ExpIfRepresentable(LogOneMinusBetaFromRapidity(currentRapidity)); }
double PhysicalObserver::GetLog10OneMinusBeta() const { return LogOneMinusBetaFromRapidity(currentRapidity) / Ln10; }
double PhysicalObserver::GetSpeed() const { return PhysicsConstants::C * GetBeta(); }
double PhysicalObserver::GetGamma() const { return ExpIfRepresentable(LogGammaFromRapidity(currentRapidity)); }
double PhysicalObserver::GetLog10Gamma() const { return LogGammaFromRapidity(currentRapidity) / Ln10; }
double PhysicalObserver::GetRapidity() const { return currentRapidity; }
double PhysicalObserver::GetProperVelocityMagnitude() const
{
    if (currentRapidity <= 0.0)
        return 0.0;

    return ExpIfRepresentable(std::log(PhysicsConstants::C) + LogSinhFromRapidity(currentRapidity));
}
double PhysicalObserver::GetLog10ProperVelocityC() const
{
    if (currentRapidity <= 0.0)
        return -std::numeric_limits<double>::infinity();

    return LogSinhFromRapidity(currentRapidity) / Ln10;
}
double PhysicalObserver::GetTimeRate() const { return ExpIfRepresentable(-LogGammaFromRapidity(currentRapidity)); }
double PhysicalObserver::GetProperTime() const { return properTime; }
double PhysicalObserver::GetMaxGamma() const { return std::numeric_limits<double>::infinity(); }
