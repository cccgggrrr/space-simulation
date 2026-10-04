#pragma once

#include <vector>
#include <GL/glew.h>
#include <glm/glm.hpp>

struct CatalogStar
{
    glm::dvec3 position;
    float magnitude;
    float temperature;
};

class StarField
{
public:
    StarField(int starCount);
    ~StarField();

    void Render() const;

private:
    struct StarVertex
    {
        glm::vec3 position;
        glm::vec3 color;
        float brightness;
        float size;
    };

    GLuint VAO = 0;
    GLuint VBO = 0;

    std::vector<CatalogStar> stars;
    std::vector<StarVertex> vertices;

    void GenerateTemporaryStars(int starCount);
    void CreateRenderData();
    glm::vec3 TemperatureToColor(float temperature) const;
};