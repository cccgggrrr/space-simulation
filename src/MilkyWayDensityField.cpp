#include "MilkyWayDensityField.h"

#include <algorithm>
#include <cmath>

#include "GalacticConstants.h"
#include "MilkyWayModel.h"

namespace
{
    constexpr double SolarNeighborhoodDensity = 0.081;
    constexpr double DiskScaleLength = 2600.0;
    constexpr double ThinDiskScaleHeight = 300.0;
    constexpr double CloudSpawnThreshold = 0.260;
}

double MilkyWayDensityField::DiskHalfThickness(double radius) const
{
    double modelHeight = MilkyWayModel::DiskScaleHeight(radius);
    return std::clamp(modelHeight * 1.20, 300.0, 590.0);
}

double MilkyWayDensityField::SampleCloudDensity(const glm::dvec3& p) const
{
    double radius = std::sqrt(p.x * p.x + p.y * p.y);

    if (radius >= GalacticConstants::MilkyWayDiskRadiusParsec)
        return 0.0;

    return MilkyWayModel::CloudDensity(
        p.x,
        p.y,
        p.z,
        GalacticConstants::MilkyWayDiskRadiusParsec
    );
}

double MilkyWayDensityField::SampleStarDensity(const glm::dvec3& p) const
{
    double radius = std::sqrt(p.x * p.x + p.y * p.y);

    if (radius >= GalacticConstants::MilkyWayDiskRadiusParsec)
        return 0.0;

    double cloud = SampleCloudDensity(p);

    if (cloud < CloudSpawnThreshold)
        return 0.0;

    bool insideDisk = std::abs(p.z) <= DiskHalfThickness(radius);
    bool insideBulge = radius <= 3100.0 && std::abs(p.z) <= 1100.0;

    if (!insideDisk && !insideBulge)
        return 0.0;

    glm::dvec3 sunPosition(
        -GalacticConstants::SunToGalacticCenterParsec,
        0.0,
        GalacticConstants::SunHeightAboveGalacticPlaneParsec
    );

    auto rawDiskDensity = [this](const glm::dvec3& q)
    {
        double r = std::sqrt(q.x * q.x + q.y * q.y);

        if (r >= GalacticConstants::MilkyWayDiskRadiusParsec || std::abs(q.z) > DiskHalfThickness(r))
            return 0.0;

        double angle = std::atan2(q.y, q.x);
        double arm = MilkyWayModel::ArmDensity(r, angle);
        double radial = std::exp((GalacticConstants::SunToGalacticCenterParsec - r) / DiskScaleLength);
        double vertical = std::exp(-std::abs(q.z) / ThinDiskScaleHeight);
        double armFactor = 0.50 + 1.45 * std::pow(std::clamp(arm, 0.0, 1.0), 0.60);
        double edge = MilkyWayModel::DiskEdgeTaper(r, GalacticConstants::MilkyWayDiskRadiusParsec);
        return radial * vertical * armFactor * edge;
    };

    double normalization = rawDiskDensity(sunPosition);
    double disk = normalization > 0.0 ? SolarNeighborhoodDensity * rawDiskDensity(p) / normalization : 0.0;

    double bulge = 0.0;
    double core = 0.0;

    if (insideBulge)
    {
        bulge = 2.4 * MilkyWayModel::BulgeDensity(p.x, p.y, p.z);
        core = 10.0 * MilkyWayModel::CoreDensity(p.x, p.y, p.z);
    }

    double cloudGate = MilkyWayModel::Smooth01((cloud - CloudSpawnThreshold) / 0.30);
    double cloudVariation = 0.18 + 1.82 * std::pow(cloud, 1.35);

    return std::max(0.0, (disk + bulge + core) * cloudGate * cloudVariation);
}

bool MilkyWayDensityField::IsInsideStellarVolume(const glm::dvec3& p) const
{
    double radius = std::sqrt(p.x * p.x + p.y * p.y);

    if (radius >= GalacticConstants::MilkyWayDiskRadiusParsec)
        return false;

    double cloud = SampleCloudDensity(p);

    if (cloud < CloudSpawnThreshold)
        return false;

    bool insideDisk = std::abs(p.z) <= DiskHalfThickness(radius);
    bool insideBulge = radius <= 3100.0 && std::abs(p.z) <= 1100.0;

    return insideDisk || insideBulge;
}

double MilkyWayDensityField::GetSolarNeighborhoodDensity() const
{
    return SolarNeighborhoodDensity;
}
