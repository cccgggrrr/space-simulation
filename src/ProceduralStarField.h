#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class ProceduralStarField
{
public:
    explicit ProceduralStarField(std::size_t starCount);
    ~ProceduralStarField();

    ProceduralStarField(const ProceduralStarField&) = delete;
    ProceduralStarField& operator=(const ProceduralStarField&) = delete;

    void Render() const;
    std::size_t GetStarCount() const;

private:
#pragma pack(push, 1)
    struct StarVertex
    {
        float PositionHigh[3];
        std::uint32_t PackedColor;
        std::int16_t MagnitudeQ;
        std::int16_t PositionLowQ[3];
    };
#pragma pack(pop)

    std::vector<StarVertex> stars;
    std::size_t starCount = 0;
    GLuint vao = 0;
    GLuint vbo = 0;

    void Generate(std::size_t requestedStarCount);
    void CreateBuffers();
};
