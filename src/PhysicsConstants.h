#pragma once

namespace PhysicsConstants
{
    inline constexpr double G = 6.67430e-11;
    inline constexpr double C = 299792458.0;
    inline constexpr double StandardGravity = 9.80665;

    inline constexpr double SolarMass = 1.98847e30;
    inline constexpr double SolarRadius = 6.957e8;

    inline constexpr double EarthMass = 5.9722e24;
    inline constexpr double EarthRadius = 6.371e6;

    inline constexpr double AU = 1.495978707e11;
    inline constexpr double JulianYear = 31557600.0;

    inline double SchwarzschildRadius(double mass)
    {
        return 2.0 * G * mass / (C * C);
    }
}