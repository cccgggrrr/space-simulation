#pragma once

#include <string>
#include <glm/glm.hpp>

enum class CelestialBodyType
{
    Planet,
    Star,
    NeutronStar,
    BlackHole
};

class CelestialBody
{
public:
    std::string Name;
    CelestialBodyType Type;

    double Mass;
    double Radius;
    bool ShowNameTag = false;

    glm::dvec3 Position;
    glm::dvec3 Velocity;

    CelestialBody(const std::string& name, CelestialBodyType type, double mass, double radius, const glm::dvec3& position = glm::dvec3(0.0), const glm::dvec3& velocity = glm::dvec3(0.0))
        : Name(name), Type(type), Mass(mass), Radius(radius), Position(position), Velocity(velocity)
    {}
};