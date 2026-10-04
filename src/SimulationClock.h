#pragma once

class SimulationClock
{
public:
    explicit SimulationClock(double timeScale = 3600.0);

    double Advance(double realDeltaTime);
    double AdvanceCoordinate(double coordinateDeltaTime);

    double GetCoordinateTime() const;
    double GetTimeScale() const;
    bool IsPaused() const;

    void SetTimeScale(double timeScale);
    void SetPaused(bool paused);
    void TogglePause();
    void Reset(double coordinateTime = 0.0);

private:
    double coordinateTime = 0.0;
    double timeScale = 3600.0;
    bool paused = false;
};
