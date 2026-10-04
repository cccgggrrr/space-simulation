#pragma once

#include <algorithm>
#include <array>
#include <cmath>

namespace MilkyWayModel
{
    inline constexpr double Pi = 3.14159265358979323846;
    inline constexpr double TwoPi = 2.0 * Pi;
    inline constexpr double BarAngle = 27.0 * Pi / 180.0;

    struct Arm
    {
        double Phase;
        double PitchDegrees;
        double MinRadiusParsec;
        double MaxRadiusParsec;
        double WidthInnerParsec;
        double WidthOuterParsec;
        double Strength;
        double WobblePhase;
    };

    inline constexpr std::array<Arm, 5> Arms = {
        Arm{0.12, 18.5, 2900.0, 14700.0, 760.0, 2200.0, 1.00, 0.2},
        Arm{1.67, 17.2, 3300.0, 14200.0, 800.0, 2300.0, 0.94, 1.5},
        Arm{3.20, 19.4, 3700.0, 14800.0, 840.0, 2380.0, 0.90, 2.8},
        Arm{4.78, 16.9, 2750.0, 13100.0, 720.0, 2100.0, 0.84, 4.1},
        Arm{2.55, 25.0, 6900.0, 10600.0, 430.0, 900.0, 0.30, 5.2}
    };

    inline double Smooth01(double value)
    {
        value = std::clamp(value, 0.0, 1.0);
        return value * value * (3.0 - 2.0 * value);
    }

    inline double WrapAngle(double angle)
    {
        while (angle > Pi)
            angle -= TwoPi;

        while (angle < -Pi)
            angle += TwoPi;

        return angle;
    }

    inline double ArmAngle(const Arm& arm, double radiusParsec)
    {
        double pitch = arm.PitchDegrees * Pi / 180.0;
        double referenceRadius = std::max(arm.MinRadiusParsec, 2500.0);
        double base = arm.Phase + std::log(radiusParsec / referenceRadius) / std::tan(pitch);
        double wobble = 0.075 * std::sin(radiusParsec / 1550.0 + arm.WobblePhase)
            + 0.032 * std::sin(radiusParsec / 590.0 + arm.WobblePhase * 2.1)
            + 0.014 * std::sin(radiusParsec / 245.0 + arm.WobblePhase * 3.7);
        return base + wobble;
    }

    inline double ArmWidthParsec(const Arm& arm, double radiusParsec)
    {
        double t = std::clamp((radiusParsec - arm.MinRadiusParsec) / std::max(arm.MaxRadiusParsec - arm.MinRadiusParsec, 1.0), 0.0, 1.0);
        double base = arm.WidthInnerParsec + (arm.WidthOuterParsec - arm.WidthInnerParsec) * t;
        double modulation = 0.88 + 0.16 * std::sin(radiusParsec / 1180.0 + arm.WobblePhase)
            + 0.08 * std::sin(radiusParsec / 410.0 + arm.WobblePhase * 1.8);
        return base * std::clamp(modulation, 0.70, 1.18);
    }

    inline double ArmSegmentStrength(const Arm& arm, double radiusParsec)
    {
        double broad = 0.66 + 0.20 * std::sin(radiusParsec / 980.0 + arm.WobblePhase);
        double medium = 0.12 * std::sin(radiusParsec / 365.0 + arm.WobblePhase * 2.4);
        double fine = 0.06 * std::sin(radiusParsec / 155.0 + arm.WobblePhase * 4.3);
        return std::clamp(broad + medium + fine, 0.22, 1.0);
    }

    inline double ArmDensity(double radiusParsec, double angle)
    {
        double result = 0.0;

        for (const Arm& arm : Arms)
        {
            if (radiusParsec < arm.MinRadiusParsec || radiusParsec > arm.MaxRadiusParsec)
                continue;

            double center = ArmAngle(arm, radiusParsec);
            double angularDifference = WrapAngle(angle - center);
            double physicalDistance = std::abs(angularDifference) * radiusParsec;
            double width = ArmWidthParsec(arm, radiusParsec);
            double core = std::exp(-0.5 * physicalDistance * physicalDistance / (width * width));
            double shoulderWidth = width * 2.35;
            double shoulder = std::exp(-0.5 * physicalDistance * physicalDistance / (shoulderWidth * shoulderWidth));
            double profile = core * 0.66 + shoulder * 0.34;
            double value = profile * arm.Strength * ArmSegmentStrength(arm, radiusParsec);
            result = std::max(result, value);
        }

        return std::clamp(result, 0.0, 1.0);
    }

    inline double DiskEdgeTaper(double radiusParsec, double diskRadiusParsec)
    {
        constexpr double taperWidth = 3400.0;

        if (radiusParsec <= diskRadiusParsec - taperWidth)
            return 1.0;

        if (radiusParsec >= diskRadiusParsec)
            return 0.0;

        double t = (diskRadiusParsec - radiusParsec) / taperWidth;
        return Smooth01(t);
    }

    inline double DiskScaleHeight(double radiusParsec)
    {
        double central = 260.0 * std::exp(-radiusParsec / 2800.0);
        double flare = 210.0 * Smooth01((radiusParsec - 6500.0) / 8000.0);
        return 270.0 + central + flare;
    }

    inline double BulgeDensity(double x, double y, double z)
    {
        double c = std::cos(BarAngle);
        double s = std::sin(BarAngle);
        double xr = x * c + y * s;
        double yr = -x * s + y * c;
        double q = xr * xr / (2500.0 * 2500.0)
            + yr * yr / (1050.0 * 1050.0)
            + z * z / (820.0 * 820.0);
        return std::exp(-0.5 * q);
    }

    inline double CoreDensity(double x, double y, double z)
    {
        double c = std::cos(BarAngle);
        double s = std::sin(BarAngle);
        double xr = x * c + y * s;
        double yr = -x * s + y * c;
        double q = xr * xr / (1350.0 * 1350.0)
            + yr * yr / (620.0 * 620.0)
            + z * z / (520.0 * 520.0);
        return std::exp(-0.5 * q);
    }

    inline double DiskVerticalDensity(double radiusParsec, double zParsec)
    {
        double height = DiskScaleHeight(radiusParsec);
        return std::exp(-std::abs(zParsec) / std::max(height, 1.0));
    }

    inline double CloudDensity(double x, double y, double z, double diskRadiusParsec)
    {
        double radius = std::sqrt(x * x + y * y);

        if (radius >= diskRadiusParsec)
            return 0.0;

        double angle = std::atan2(y, x);
        double arm = ArmDensity(radius, angle);
        double edge = DiskEdgeTaper(radius, diskRadiusParsec);
        double vertical = DiskVerticalDensity(radius, z);
        double outer = Smooth01((radius - 5200.0) / 9000.0);
        double innerBlend = 1.0 - Smooth01((radius - 2500.0) / 3600.0);

        double baseline = 0.20 * (1.0 - outer) + 0.070 * outer;
        double softenedArm = std::pow(std::clamp(arm, 0.0, 1.0), 0.46 + 0.20 * (1.0 - innerBlend));
        double armField = (0.58 + 0.08 * outer) * softenedArm;
        double interArmFill = (0.11 + 0.04 * innerBlend) * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.78));
        double bulge = 0.72 * BulgeDensity(x, y, z);
        double diskField = baseline + armField + interArmFill;

        return std::clamp((diskField * vertical + bulge) * edge, 0.0, 1.0);
    }
}
