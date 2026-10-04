#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

struct NamedStar
{
    std::string Name;
    std::string SpectralType;
    double GalacticLongitudeDegrees = 0.0;
    double GalacticLatitudeDegrees = 0.0;
    double DistanceParsec = 0.0;
    glm::dvec3 PositionParsecs = glm::dvec3(0.0);
    glm::vec3 Color = glm::vec3(1.0f);
};

class NamedStarField
{
public:
    NamedStarField();
    ~NamedStarField();

    NamedStarField(const NamedStarField&) = delete;
    NamedStarField& operator=(const NamedStarField&) = delete;

    void Render(const glm::dvec3& cameraLocalPositionMeters) const;

    const std::vector<NamedStar>& GetStars() const;
    glm::dvec3 GetLocalPositionMeters(std::size_t index) const;

private:
    struct RenderStar
    {
        glm::vec3 Position;
        glm::vec3 Color;
    };

    std::vector<NamedStar> stars;
    mutable std::vector<RenderStar> renderStars;

    GLuint vao = 0;
    GLuint vbo = 0;

    void AddStar(const std::string& name, const std::string& spectralType, double lDegrees, double bDegrees, double distanceParsec, const glm::vec3& color);
    void CreateBuffers();
};
