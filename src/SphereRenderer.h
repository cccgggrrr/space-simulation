#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader;
class Camera;
class CelestialBody;
class Texture2D;

class SphereRenderer
{
public:
    SphereRenderer(unsigned int latitudeSegments = 64, unsigned int longitudeSegments = 64);
    ~SphereRenderer();

    void Render(const CelestialBody& body, const Camera& camera, Shader& shader, const glm::mat4& projection, const Texture2D* texture = nullptr) const;

private:
    GLuint VAO = 0;
    GLuint VBO = 0;
    GLuint EBO = 0;

    GLsizei indexCount = 0;

    glm::vec3 GetColor(const CelestialBody& body) const;
};