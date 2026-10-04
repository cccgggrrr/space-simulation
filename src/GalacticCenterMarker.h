#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

class GalacticCenterMarker
{
public:
    GalacticCenterMarker();
    ~GalacticCenterMarker();

    GalacticCenterMarker(const GalacticCenterMarker&) = delete;
    GalacticCenterMarker& operator=(const GalacticCenterMarker&) = delete;

    void Render(const glm::dvec3& cameraLocalPositionMeters);

private:
    GLuint vao = 0;
    GLuint vbo = 0;
    glm::vec3 renderPosition = glm::vec3(0.0f);
};
