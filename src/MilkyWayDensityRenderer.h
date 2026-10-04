#pragma once

#include <cstddef>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class MilkyWayDensityRenderer
{
public:
    explicit MilkyWayDensityRenderer(std::size_t particleCount = 470000);
    ~MilkyWayDensityRenderer();

    MilkyWayDensityRenderer(const MilkyWayDensityRenderer&) = delete;
    MilkyWayDensityRenderer& operator=(const MilkyWayDensityRenderer&) = delete;

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
