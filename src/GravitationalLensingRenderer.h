#pragma once

#include <string>
#include <vector>

#include <GL/glew.h>
#include <glm/glm.hpp>

class CelestialBody;

class GravitationalLensingRenderer
{
public:
    GravitationalLensingRenderer();
    ~GravitationalLensingRenderer();

    GravitationalLensingRenderer(const GravitationalLensingRenderer&) = delete;
    GravitationalLensingRenderer& operator=(const GravitationalLensingRenderer&) = delete;

    void Begin(int width, int height);
    void EndAndRender(const std::vector<CelestialBody>& bodies, const glm::dvec3& cameraPosition, const glm::mat4& view, const glm::mat4& projection, int width, int height);

private:
    static constexpr int MaxLenses = 8;

    GLuint framebuffer = 0;
    GLuint colorTexture = 0;
    GLuint depthStencil = 0;
    GLuint vao = 0;
    GLuint program = 0;
    int bufferWidth = 0;
    int bufferHeight = 0;

    void Resize(int width, int height);
    GLuint CompileShader(GLenum type, const std::string& source);
    GLuint CreateProgram(const char* vertexPath, const char* fragmentPath);
    std::string ReadFile(const char* path);
};
