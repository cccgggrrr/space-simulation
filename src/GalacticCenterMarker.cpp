#include "GalacticCenterMarker.h"

#include "GalacticFrame.h"

GalacticCenterMarker::GalacticCenterMarker()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(glm::vec3), nullptr);

    glBindVertexArray(0);
}

GalacticCenterMarker::~GalacticCenterMarker()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void GalacticCenterMarker::Render(const glm::dvec3& cameraLocalPositionMeters)
{
    glm::dvec3 cameraGalacticParsecs = GalacticFrame::LocalMetersToGalactocentricParsecs(cameraLocalPositionMeters);
    renderPosition = glm::vec3(-cameraGalacticParsecs);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(glm::vec3), &renderPosition);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, 1);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}
