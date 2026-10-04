#pragma once

#include <glm/glm.hpp>

class MilkyWayDensityField
{
public:
    double SampleCloudDensity(const glm::dvec3& galactocentricParsecs) const;
    double SampleStarDensity(const glm::dvec3& galactocentricParsecs) const;
    bool IsInsideStellarVolume(const glm::dvec3& galactocentricParsecs) const;
    double GetSolarNeighborhoodDensity() const;

private:
    double DiskHalfThickness(double radiusParsecs) const;
};
