#pragma once

#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class MilkyWayOutline
{
public:
    MilkyWayOutline();
    ~MilkyWayOutline();

    MilkyWayOutline(const MilkyWayOutline&) = delete;
    MilkyWayOutline& operator=(const MilkyWayOutline&) = delete;

    void Render(const glm::dvec3& cameraLocalPositionMeters);

private:
    std::vector<glm::dvec3> galacticVerticesParsecs;
    std::vector<glm::vec3> renderVertices;

    GLuint vao = 0;
    GLuint vbo = 0;

    void Build();
    void CreateBuffers();
};
