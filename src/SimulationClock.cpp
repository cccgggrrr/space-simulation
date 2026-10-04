#include "SimulationClock.h"

#include <algorithm>

SimulationClock::SimulationClock(double timeScale)
{
    SetTimeScale(timeScale);
}

double SimulationClock::Advance(double realDeltaTime)
{
    if (paused || realDeltaTime <= 0.0)
        return 0.0;

    double simulationDelta = realDeltaTime * timeScale;
    coordinateTime += simulationDelta;

    return simulationDelta;
}

double SimulationClock::AdvanceCoordinate(double coordinateDeltaTime)
{
    if (paused || coordinateDeltaTime <= 0.0)
        return 0.0;

    coordinateTime += coordinateDeltaTime;
    return coordinateDeltaTime;
}

double SimulationClock::GetCoordinateTime() const
{
    return coordinateTime;
}

double SimulationClock::GetTimeScale() const
{
    return timeScale;
}

bool SimulationClock::IsPaused() const
{
    return paused;
}

void SimulationClock::SetTimeScale(double value)
{
    timeScale = std::max(0.0, value);
}

void SimulationClock::SetPaused(bool value)
{
    paused = value;
}

void SimulationClock::TogglePause()
{
    paused = !paused;
}

void SimulationClock::Reset(double value)
{
    coordinateTime = std::max(0.0, value);
}
