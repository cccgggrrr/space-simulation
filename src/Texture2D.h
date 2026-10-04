#pragma once

#include <string>
#include <GL/glew.h>

class Texture2D
{
public:
    Texture2D(const std::string& path);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;

    void Bind(unsigned int unit = 0) const;
    bool IsValid() const;

private:
    GLuint ID = 0;
};