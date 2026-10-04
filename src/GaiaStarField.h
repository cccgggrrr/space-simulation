#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class GaiaStarField
{
public:
    explicit GaiaStarField(const std::string& csvPath);
    ~GaiaStarField();

    GaiaStarField(const GaiaStarField&) = delete;
    GaiaStarField& operator=(const GaiaStarField&) = delete;

    void Render(const glm::dvec3& cameraPosition);
    std::size_t GetStarCount() const;

private:
    struct Star
    {
        std::uint64_t SourceId = 0;
        glm::dvec3 Position = glm::dvec3(0.0);
        glm::vec3 Color = glm::vec3(1.0f);
        double AbsoluteMagnitude = 0.0;
    };

    struct RenderStar
    {
        glm::vec3 Position;
        glm::vec3 Color;
        float Magnitude;
    };

    std::vector<Star> stars;
    std::vector<RenderStar> renderStars;
    GLuint vao = 0;
    GLuint vbo = 0;

    void Load(const std::string& csvPath);
    void CreateBuffers();
    static std::vector<std::string> ParseCsvLine(const std::string& line);
    static glm::vec3 BpRpToColor(double bpRp);
};
