#include "StarField.h"

#include <cmath>
#include <cstddef>
#include <random>

StarField::StarField(int starCount)
{
    GenerateTemporaryStars(starCount);
    CreateRenderData();

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(StarVertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, position)));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, color)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, brightness)));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, size)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);
}

StarField::~StarField()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void StarField::GenerateTemporaryStars(int starCount)
{
    std::mt19937 random(42);

    std::uniform_real_distribution<double> radiusDistribution(30.0, 300.0);
    std::uniform_real_distribution<double> zDistribution(-1.0, 1.0);
    std::uniform_real_distribution<double> angleDistribution(0.0, 6.283185307179586);
    std::uniform_real_distribution<float> magnitudeDistribution(-1.0f, 6.0f);
    std::uniform_real_distribution<float> temperatureDistribution(2500.0f, 12000.0f);

    stars.reserve(starCount);

    for (int i = 0; i < starCount; i++)
    {
        double z = zDistribution(random);
        double angle = angleDistribution(random);
        double horizontal = std::sqrt(1.0 - z * z);
        double radius = radiusDistribution(random);

        glm::dvec3 direction(horizontal * std::cos(angle), z, horizontal * std::sin(angle));

        CatalogStar star;
        star.position = direction * radius;
        star.magnitude = magnitudeDistribution(random);
        star.temperature = temperatureDistribution(random);

        stars.push_back(star);
    }
}

glm::vec3 StarField::TemperatureToColor(float temperature) const
{
    if (temperature < 3500.0f)
        return glm::vec3(1.0f, 0.55f, 0.35f);

    if (temperature < 5000.0f)
        return glm::vec3(1.0f, 0.78f, 0.55f);

    if (temperature < 6500.0f)
        return glm::vec3(1.0f, 0.95f, 0.85f);

    if (temperature < 8500.0f)
        return glm::vec3(0.85f, 0.90f, 1.0f);

    return glm::vec3(0.65f, 0.78f, 1.0f);
}

void StarField::CreateRenderData()
{
    vertices.reserve(stars.size());

    for (const CatalogStar& star : stars)
    {
        float brightness = glm::clamp((6.0f - star.magnitude) / 7.0f, 0.15f, 1.0f);
        float size = 1.0f + brightness * 3.0f;

        StarVertex vertex;
        vertex.position = glm::vec3(star.position);
        vertex.color = TemperatureToColor(star.temperature);
        vertex.brightness = brightness;
        vertex.size = size;

        vertices.push_back(vertex);
    }
}

void StarField::Render() const
{
    glBindVertexArray(VAO);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(vertices.size()));
    glBindVertexArray(0);
}