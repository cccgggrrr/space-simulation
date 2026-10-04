#include "GravitationalLensingRenderer.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include <glm/gtc/type_ptr.hpp>

#include "CelestialBody.h"
#include "PhysicsConstants.h"
#include "RenderConstants.h"

GravitationalLensingRenderer::GravitationalLensingRenderer()
{
    program = CreateProgram("shaders/lensing.vert", "shaders/lensing.frag");
    glGenFramebuffers(1, &framebuffer);
    glGenTextures(1, &colorTexture);
    glGenRenderbuffers(1, &depthStencil);
    glGenVertexArrays(1, &vao);
}

GravitationalLensingRenderer::~GravitationalLensingRenderer()
{
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);

    if (depthStencil != 0)
        glDeleteRenderbuffers(1, &depthStencil);

    if (colorTexture != 0)
        glDeleteTextures(1, &colorTexture);

    if (framebuffer != 0)
        glDeleteFramebuffers(1, &framebuffer);

    if (program != 0)
        glDeleteProgram(program);
}

void GravitationalLensingRenderer::Begin(int width, int height)
{
    if (width <= 0 || height <= 0)
        return;

    Resize(width, height);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
}

void GravitationalLensingRenderer::EndAndRender(const std::vector<CelestialBody>& bodies, const glm::dvec3& cameraPosition, const glm::mat4& view, const glm::mat4& projection, int width, int height)
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, width, height);

    if (width <= 0 || height <= 0)
        return;

    glm::vec4 lenses[MaxLenses]{};
    int lensCount = 0;
    double verticalFov = glm::radians(60.0);
    double pixelsPerRadian = static_cast<double>(height) / verticalFov;

    for (const CelestialBody& body : bodies)
    {
        if (body.Type != CelestialBodyType::BlackHole || lensCount >= MaxLenses)
            continue;

        glm::dvec3 relativeMeters = body.Position - cameraPosition;
        double distanceMeters = glm::length(relativeMeters);

        if (distanceMeters <= std::max(body.Radius * 1.01, 1.0))
            continue;

        glm::vec3 renderPosition = glm::vec3(relativeMeters * RenderConstants::WorldToRenderScale);
        glm::vec4 clip = projection * view * glm::vec4(renderPosition, 1.0f);

        if (clip.w <= 0.0f)
            continue;

        glm::vec3 ndc = glm::vec3(clip) / clip.w;
        glm::vec2 uv(ndc.x * 0.5f + 0.5f, ndc.y * 0.5f + 0.5f);

        double schwarzschildRadius = PhysicsConstants::SchwarzschildRadius(body.Mass);
        double einsteinAngle = std::sqrt(std::max(2.0 * schwarzschildRadius / distanceMeters, 0.0));
        double physicalEinsteinPixels = einsteinAngle * pixelsPerRadian;
        double displayEinsteinPixels = std::clamp(physicalEinsteinPixels * 26.0, 0.0, 180.0);

        if (displayEinsteinPixels < 0.45)
            continue;

        double physicalShadowAngle = 2.598076211 * schwarzschildRadius / distanceMeters;
        double physicalShadowPixels = physicalShadowAngle * pixelsPerRadian;
        double displayShadowPixels = std::clamp(std::max(physicalShadowPixels * 220.0, displayEinsteinPixels * 0.12), 0.0, 42.0);

        double edgeMarginX = displayEinsteinPixels * 3.6 / static_cast<double>(width);
        double edgeMarginY = displayEinsteinPixels * 3.6 / static_cast<double>(height);

        if (uv.x < -edgeMarginX || uv.x > 1.0 + edgeMarginX || uv.y < -edgeMarginY || uv.y > 1.0 + edgeMarginY)
            continue;

        lenses[lensCount++] = glm::vec4(
            uv.x,
            uv.y,
            static_cast<float>(displayEinsteinPixels),
            static_cast<float>(displayShadowPixels)
        );
    }

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_BLEND);

    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glUniform1i(glGetUniformLocation(program, "sceneTexture"), 0);
    glUniform2f(glGetUniformLocation(program, "viewportSize"), static_cast<float>(width), static_cast<float>(height));
    glUniform1i(glGetUniformLocation(program, "lensCount"), lensCount);

    if (lensCount > 0)
        glUniform4fv(glGetUniformLocation(program, "lenses"), lensCount, glm::value_ptr(lenses[0]));

    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void GravitationalLensingRenderer::Resize(int width, int height)
{
    if (width == bufferWidth && height == bufferHeight)
        return;

    bufferWidth = width;
    bufferHeight = height;

    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindRenderbuffer(GL_RENDERBUFFER, depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTexture, 0);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthStencil);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        throw std::runtime_error("Gravitational lensing framebuffer is incomplete");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint GravitationalLensingRenderer::CompileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* sourcePointer = source.c_str();
    glShaderSource(shader, 1, &sourcePointer, nullptr);
    glCompileShader(shader);

    GLint success = GL_FALSE;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

    if (success == GL_TRUE)
        return shader;

    GLint length = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);

    std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
    glGetShaderInfoLog(shader, length, nullptr, log.data());
    glDeleteShader(shader);
    throw std::runtime_error(log);
}

GLuint GravitationalLensingRenderer::CreateProgram(const char* vertexPath, const char* fragmentPath)
{
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, ReadFile(vertexPath));
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, ReadFile(fragmentPath));

    GLuint result = glCreateProgram();
    glAttachShader(result, vertexShader);
    glAttachShader(result, fragmentShader);
    glLinkProgram(result);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    GLint success = GL_FALSE;
    glGetProgramiv(result, GL_LINK_STATUS, &success);

    if (success == GL_TRUE)
        return result;

    GLint length = 0;
    glGetProgramiv(result, GL_INFO_LOG_LENGTH, &length);

    std::string log(static_cast<std::size_t>(std::max(length, 1)), '\0');
    glGetProgramInfoLog(result, length, nullptr, log.data());
    glDeleteProgram(result);
    throw std::runtime_error(log);
}

std::string GravitationalLensingRenderer::ReadFile(const char* path)
{
    std::ifstream file(path);

    if (!file)
        throw std::runtime_error(std::string("Failed to open shader: ") + path);

    std::ostringstream stream;
    stream << file.rdbuf();
    return stream.str();
}
