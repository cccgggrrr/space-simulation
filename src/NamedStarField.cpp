#include "NamedStarField.h"

#include <cmath>

#include "GalacticConstants.h"

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double DegToRad = Pi / 180.0;
}

NamedStarField::NamedStarField()
{
    AddStar("Sirius", "A0mA1Va", 227.23029126, -8.89028121, 2.6370612589, glm::vec3(0.72f, 0.82f, 1.0f));
    AddStar("Polaris", "F8Ib", 123.28054967, 26.46139598, 132.6259946950, glm::vec3(1.0f, 0.92f, 0.72f));
    AddStar("Proxima Centauri", "M5.5Ve", 313.93986205, -1.92714913, 1.3019705976, glm::vec3(1.0f, 0.42f, 0.28f));
    AddStar("Betelgeuse", "M1-M2Ia-Iab", 199.78723619, -8.95860329, 152.6717557252, glm::vec3(1.0f, 0.38f, 0.18f));
    AddStar("Rigel", "B8Ia", 209.24118506, -25.24534825, 264.5502645503, glm::vec3(0.62f, 0.76f, 1.0f));
    AddStar("Vega", "A0V", 67.44820813, 19.23725227, 7.6787222606, glm::vec3(0.72f, 0.84f, 1.0f));
    AddStar("Antares", "M1.5Iab+B2Vn", 351.94713737, 15.06432170, 169.7792869270, glm::vec3(1.0f, 0.35f, 0.16f));
    AddStar("UY Scuti", "M4Iae", 19.14357862, -0.47213952, 1935.7336430507, glm::vec3(1.0f, 0.28f, 0.12f));

    renderStars.resize(stars.size());
    CreateBuffers();
}

NamedStarField::~NamedStarField()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void NamedStarField::AddStar(const std::string& name, const std::string& spectralType, double lDegrees, double bDegrees, double distanceParsec, const glm::vec3& color)
{
    double l = lDegrees * DegToRad;
    double b = bDegrees * DegToRad;
    double cosB = std::cos(b);

    NamedStar star;
    star.Name = name;
    star.SpectralType = spectralType;
    star.GalacticLongitudeDegrees = lDegrees;
    star.GalacticLatitudeDegrees = bDegrees;
    star.DistanceParsec = distanceParsec;
    star.PositionParsecs = glm::dvec3(
        distanceParsec * cosB * std::cos(l),
        distanceParsec * cosB * std::sin(l),
        distanceParsec * std::sin(b)
    );
    star.Color = color;

    stars.push_back(star);
}

void NamedStarField::CreateBuffers()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, renderStars.size() * sizeof(RenderStar), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderStar), reinterpret_cast<void*>(offsetof(RenderStar, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderStar), reinterpret_cast<void*>(offsetof(RenderStar, Color)));

    glBindVertexArray(0);
}

void NamedStarField::Render(const glm::dvec3& cameraLocalPositionMeters) const
{
    glm::dvec3 cameraParsecs = cameraLocalPositionMeters / GalacticConstants::Parsec;

    for (std::size_t i = 0; i < stars.size(); i++)
    {
        renderStars[i].Position = glm::vec3(stars[i].PositionParsecs - cameraParsecs);
        renderStars[i].Color = stars[i].Color;
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, renderStars.size() * sizeof(RenderStar), renderStars.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(renderStars.size()));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

const std::vector<NamedStar>& NamedStarField::GetStars() const
{
    return stars;
}

glm::dvec3 NamedStarField::GetLocalPositionMeters(std::size_t index) const
{
    if (index >= stars.size())
        return glm::dvec3(0.0);

    return stars[index].PositionParsecs * GalacticConstants::Parsec;
}
