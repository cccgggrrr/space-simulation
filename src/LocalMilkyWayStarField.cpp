#include "LocalMilkyWayStarField.h"

LocalMilkyWayStarField::LocalMilkyWayStarField()
{
    glGenVertexArrays(1, &vao);
}

LocalMilkyWayStarField::~LocalMilkyWayStarField()
{
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void LocalMilkyWayStarField::Render() const
{
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_POINTS, 0, CandidatesPerChunk, ChunkCount);
    glBindVertexArray(0);
}

std::size_t LocalMilkyWayStarField::GetCandidateCount() const
{
    return static_cast<std::size_t>(CandidatesPerChunk) * static_cast<std::size_t>(ChunkCount);
}

double LocalMilkyWayStarField::GetRangeParsecs() const
{
    return 300.0;
}
