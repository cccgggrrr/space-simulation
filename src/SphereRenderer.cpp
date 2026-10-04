#include "SphereRenderer.h"

#include <cmath>
#include <vector>

#include <glm/gtc/matrix_transform.hpp>

#include "Shader.h"
#include "Camera.h"
#include "CelestialBody.h"
#include "Texture2D.h"
#include "RenderConstants.h"

SphereRenderer::SphereRenderer(unsigned int latitudeSegments, unsigned int longitudeSegments)
{
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const double pi = 3.14159265358979323846;

    for (unsigned int y = 0; y <= latitudeSegments; y++)
    {
        double v = static_cast<double>(y) / latitudeSegments;
        double phi = v * pi;

        for (unsigned int x = 0; x <= longitudeSegments; x++)
        {
            double u = static_cast<double>(x) / longitudeSegments;
            double theta = u * 2.0 * pi;

            float px = static_cast<float>(std::sin(phi) * std::cos(theta));
            float py = static_cast<float>(std::cos(phi));
            float pz = static_cast<float>(std::sin(phi) * std::sin(theta));

            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);

            vertices.push_back(px);
            vertices.push_back(py);
            vertices.push_back(pz);
            
            vertices.push_back(static_cast<float>(1.0 - u));
            vertices.push_back(static_cast<float>(1.0 - v));
        }
    }

    for (unsigned int y = 0; y < latitudeSegments; y++)
    {
        for (unsigned int x = 0; x < longitudeSegments; x++)
        {
            unsigned int a = y * (longitudeSegments + 1) + x;
            unsigned int b = a + longitudeSegments + 1;

            indices.push_back(a);
            indices.push_back(b);
            indices.push_back(a + 1);

            indices.push_back(b);
            indices.push_back(b + 1);
            indices.push_back(a + 1);
        }
    }

    indexCount = static_cast<GLsizei>(indices.size());

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), reinterpret_cast<void*>(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
}

SphereRenderer::~SphereRenderer()
{
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
}

glm::vec3 SphereRenderer::GetColor(const CelestialBody& body) const
{
    switch (body.Type)
    {
    case CelestialBodyType::Planet:
        return glm::vec3(0.15f, 0.35f, 0.9f);

    case CelestialBodyType::Star:
        return glm::vec3(1.0f, 0.8f, 0.3f);

    case CelestialBodyType::NeutronStar:
        return glm::vec3(0.6f, 0.8f, 1.0f);

    case CelestialBodyType::BlackHole:
        return glm::vec3(0.01f);
    }

    return glm::vec3(1.0f);
}

void SphereRenderer::Render(const CelestialBody& body, const Camera& camera, Shader& shader, const glm::mat4& projection, const Texture2D* texture) const
{
    glm::dvec3 relativePosition = body.Position - camera.Position;
    glm::vec3 renderPosition = glm::vec3(relativePosition * RenderConstants::WorldToRenderScale);
    float renderRadius = static_cast<float>(body.Radius * RenderConstants::WorldToRenderScale);

    glm::mat4 model = glm::translate(glm::mat4(1.0f), renderPosition);
    model = glm::scale(model, glm::vec3(renderRadius));

    bool useTexture = texture != nullptr && texture->IsValid();

    shader.Use();
    shader.SetMat4("model", model);
    shader.SetMat4("view", camera.GetViewRotationMatrix());
    shader.SetMat4("projection", projection);
    shader.SetVec3("bodyColor", GetColor(body));
    shader.SetInt("useTexture", useTexture ? 1 : 0);
    shader.SetInt("albedoTexture", 0);

    if (useTexture)
        texture->Bind(0);

    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, indexCount, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}