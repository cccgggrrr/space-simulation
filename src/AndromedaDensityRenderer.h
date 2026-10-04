#pragma once

#include <cstddef>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class AndromedaDensityRenderer
{
public:
    explicit AndromedaDensityRenderer(std::size_t particleCount = 260000);
    ~AndromedaDensityRenderer();

    AndromedaDensityRenderer(const AndromedaDensityRenderer&) = delete;
    AndromedaDensityRenderer& operator=(const AndromedaDensityRenderer&) = delete;

    void Render() const;
    void RenderClouds() const;
    void RenderDust() const;
    std::size_t GetParticleCount() const;

private:
    struct Particle
    {
        glm::vec3 Position;
        glm::vec3 Color;
        float Brightness;
        float Size;
        float Mode;
        float Seed;
    };

    std::vector<Particle> particles;
    std::size_t baseParticleCount = 0;
    std::size_t cloudParticleCount = 0;
    std::size_t dustParticleCount = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    void Build(std::size_t particleCount);
    void CreateBuffers();
};
