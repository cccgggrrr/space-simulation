#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace AndromedaModel
{
    inline constexpr double Pi = 3.14159265358979323846;
    inline constexpr double TwoPi = 2.0 * Pi;
    inline constexpr double DiskRadiusParsec = 33000.0;
    inline constexpr double BulgeRadiusParsec = 6200.0;
    inline constexpr double CoreRadiusParsec = 2100.0;
    inline constexpr double DustRadiusParsec = 18500.0;

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

    inline constexpr std::array<Arm, 4> Arms = {
        Arm{0.10, 9.5, 4200.0, 31000.0, 1800.0, 5200.0, 1.00, 0.2},
        Arm{Pi + 0.30, 8.9, 4200.0, 32000.0, 1700.0, 5400.0, 0.94, 1.3},
        Arm{1.45, 12.0, 6500.0, 25500.0, 2800.0, 6200.0, 0.36, 2.4},
        Arm{4.65, 12.8, 6800.0, 25000.0, 3000.0, 6400.0, 0.30, 3.7}
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
        double referenceRadius = std::max(arm.MinRadiusParsec, 3500.0);
        double base = arm.Phase + std::log(radiusParsec / referenceRadius) / std::tan(pitch);
        double wobble = 0.060 * std::sin(radiusParsec / 2300.0 + arm.WobblePhase)
            + 0.026 * std::sin(radiusParsec / 720.0 + arm.WobblePhase * 2.1)
            + 0.012 * std::sin(radiusParsec / 260.0 + arm.WobblePhase * 4.2);
        return base + wobble;
    }

    inline double ArmWidthParsec(const Arm& arm, double radiusParsec)
    {
        double t = std::clamp((radiusParsec - arm.MinRadiusParsec) / std::max(arm.MaxRadiusParsec - arm.MinRadiusParsec, 1.0), 0.0, 1.0);
        double base = arm.WidthInnerParsec + (arm.WidthOuterParsec - arm.WidthInnerParsec) * t;
        double modulation = 0.92 + 0.10 * std::sin(radiusParsec / 1500.0 + arm.WobblePhase)
            + 0.05 * std::sin(radiusParsec / 460.0 + arm.WobblePhase * 1.9);
        return base * std::clamp(modulation, 0.80, 1.16);
    }

    inline double ArmSegmentStrength(const Arm& arm, double radiusParsec)
    {
        double broad = 0.72 + 0.14 * std::sin(radiusParsec / 1150.0 + arm.WobblePhase);
        double medium = 0.10 * std::sin(radiusParsec / 410.0 + arm.WobblePhase * 2.3);
        double fine = 0.04 * std::sin(radiusParsec / 185.0 + arm.WobblePhase * 3.7);
        return std::clamp(broad + medium + fine, 0.30, 1.0);
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
            double shoulder = std::exp(-0.5 * physicalDistance * physicalDistance / ((width * 2.6) * (width * 2.6)));
            double profile = core * 0.52 + shoulder * 0.48;
            double value = profile * arm.Strength * ArmSegmentStrength(arm, radiusParsec);
            result = std::max(result, value);
        }

        return std::clamp(result, 0.0, 1.0);
    }

    inline double DiskEdgeTaper(double radiusParsec)
    {
        constexpr double taperWidth = 6500.0;

        if (radiusParsec <= DiskRadiusParsec - taperWidth)
            return 1.0;
        if (radiusParsec >= DiskRadiusParsec)
            return 0.0;

        double t = (DiskRadiusParsec - radiusParsec) / taperWidth;
        return Smooth01(t);
    }

    inline double DiskScaleHeight(double radiusParsec)
    {
        double coreLift = 650.0 * std::exp(-radiusParsec / 4500.0);
        double outerFlare = 380.0 * Smooth01((radiusParsec - 12000.0) / 14000.0);
        return 340.0 + coreLift + outerFlare;
    }

    inline double BulgeDensity(double x, double y, double z)
    {
        double q = x * x / (6400.0 * 6400.0)
            + y * y / (4200.0 * 4200.0)
            + z * z / (3000.0 * 3000.0);
        return std::exp(-0.5 * q);
    }

    inline double CoreDensity(double x, double y, double z)
    {
        double q = x * x / (2100.0 * 2100.0)
            + y * y / (1500.0 * 1500.0)
            + z * z / (1200.0 * 1200.0);
        return std::exp(-0.5 * q);
    }

    inline double VerticalDensity(double radiusParsec, double zParsec)
    {
        double height = DiskScaleHeight(radiusParsec);
        return std::exp(-std::abs(zParsec) / std::max(height, 1.0));
    }

    inline double CloudDensity(double x, double y, double z)
    {
        double radius = std::sqrt(x * x + y * y);
        if (radius >= DiskRadiusParsec)
            return 0.0;

        double angle = std::atan2(y, x);
        double arm = ArmDensity(radius, angle);
        double edge = DiskEdgeTaper(radius);
        double vertical = VerticalDensity(radius, z);
        double bulge = BulgeDensity(x, y, z);
        double core = CoreDensity(x, y, z);
        double outer = Smooth01((radius - 9500.0) / 17000.0);
        double inner = 1.0 - Smooth01((radius - 4200.0) / 5200.0);

        double diskBase = 0.12 * (1.0 - outer) + 0.045 * outer;
        double broadArm = 0.55 * std::pow(std::clamp(arm, 0.0, 1.0), 0.58);
        double interArm = 0.12 * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.95));
        double bulgeField = 0.72 * bulge + 0.55 * core;
        double field = (diskBase + broadArm + interArm) * vertical + bulgeField * (0.75 + 0.25 * inner);

        return std::clamp(field * edge, 0.0, 1.0);
    }

    inline glm::dvec3 CenterParsecs()
    {
        return glm::dvec3(-382641.36, 618902.51, -285986.99);
    }

    inline glm::dmat3 Orientation()
    {
        glm::dvec3 majorAxis(-0.69372805, -0.08946034, 0.71465953);
        glm::dvec3 minorAxis(-0.34853015, 0.91003810, -0.22440452);
        glm::dvec3 normal(-0.63029209, -0.40475610, -0.66249858);
        return glm::dmat3(majorAxis, minorAxis, normal);
    }
}
