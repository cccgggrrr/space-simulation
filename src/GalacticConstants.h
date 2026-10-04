#pragma once

namespace GalacticConstants
{
    inline constexpr double LightYear = 9.4607304725808e15;
    inline constexpr double Parsec = 3.0856775814913673e16;
    inline constexpr double Kiloparsec = 1000.0 * Parsec;

    inline constexpr double SunToGalacticCenterParsec = 8200.0;
    inline constexpr double SunHeightAboveGalacticPlaneParsec = 20.8;
    inline constexpr double MilkyWayDiskRadiusParsec = 15000.0;

    inline constexpr double SunToGalacticCenter = SunToGalacticCenterParsec * Parsec;
    inline constexpr double SunHeightAboveGalacticPlane = SunHeightAboveGalacticPlaneParsec * Parsec;
    inline constexpr double MilkyWayDiskRadius = MilkyWayDiskRadiusParsec * Parsec;

    inline constexpr double SagittariusASolarMasses = 4300000.0;
}
