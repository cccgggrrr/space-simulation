#pragma once

#include <glm/glm.hpp>

#include "GalacticConstants.h"

namespace GalacticFrame
{
    inline glm::dvec3 SunGalactocentricParsecs()
    {
        return glm::dvec3(
            -GalacticConstants::SunToGalacticCenterParsec,
            0.0,
            GalacticConstants::SunHeightAboveGalacticPlaneParsec
        );
    }

    inline glm::dvec3 LocalMetersToGalactocentricParsecs(const glm::dvec3& localMeters)
    {
        return SunGalactocentricParsecs() + localMeters / GalacticConstants::Parsec;
    }

    inline glm::dvec3 GalactocentricParsecsToLocalMeters(const glm::dvec3& galactocentricParsecs)
    {
        return (galactocentricParsecs - SunGalactocentricParsecs()) * GalacticConstants::Parsec;
    }

    inline glm::dvec3 GalacticCenterLocalMeters()
    {
        return GalactocentricParsecsToLocalMeters(glm::dvec3(0.0));
    }

    inline double DistanceToGalacticCenterMeters(const glm::dvec3& localMeters)
    {
        return glm::length(LocalMetersToGalactocentricParsecs(localMeters)) * GalacticConstants::Parsec;
    }
}
