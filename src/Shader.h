#pragma once

#include <string>
#include <GL/glew.h>
#include <glm/glm.hpp>

class Shader
{
public:
    GLuint ID;

    Shader(const std::string& vertexPath, const std::string& fragmentPath);
    ~Shader();

    void Use() const;
    void SetVec3(const std::string& name, const glm::vec3& value) const;
    void SetMat4(const std::string& name, const glm::mat4& matrix) const;
    void SetInt(const std::string& name, int value) const;

private:
    std::string ReadFile(const std::string& path) const;
    GLuint CompileShader(GLenum type, const std::string& source) const;
};