#include "MilkyWayOutline.h"

#include <cmath>

#include "GalacticConstants.h"
#include "GalacticFrame.h"

namespace
{
    constexpr int SegmentCount = 720;
    constexpr double Pi = 3.14159265358979323846;
}

MilkyWayOutline::MilkyWayOutline()
{
    Build();
    CreateBuffers();
}

MilkyWayOutline::~MilkyWayOutline()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void MilkyWayOutline::Build()
{
    galacticVerticesParsecs.reserve(SegmentCount);
    renderVertices.resize(SegmentCount);

    for (int i = 0; i < SegmentCount; i++)
    {
        double angle = 2.0 * Pi * static_cast<double>(i) / static_cast<double>(SegmentCount);

        galacticVerticesParsecs.emplace_back(
            GalacticConstants::MilkyWayDiskRadiusParsec * std::cos(angle),
            GalacticConstants::MilkyWayDiskRadiusParsec * std::sin(angle),
            0.0
        );
    }
}

void MilkyWayOutline::CreateBuffers()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, renderVertices.size() * sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindVertexArray(0);
}

void MilkyWayOutline::Render(const glm::dvec3& cameraLocalPositionMeters)
{
    glm::dvec3 cameraGalacticParsecs = GalacticFrame::LocalMetersToGalactocentricParsecs(cameraLocalPositionMeters);

    for (std::size_t i = 0; i < galacticVerticesParsecs.size(); i++)
        renderVertices[i] = glm::vec3(galacticVerticesParsecs[i] - cameraGalacticParsecs);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, renderVertices.size() * sizeof(glm::vec3), renderVertices.data());

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glDrawArrays(GL_LINE_LOOP, 0, static_cast<GLsizei>(renderVertices.size()));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}
