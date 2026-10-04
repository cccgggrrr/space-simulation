#include "LocalAndromedaStarField.h"

LocalAndromedaStarField::LocalAndromedaStarField()
{
    glGenVertexArrays(1, &vao);
}

LocalAndromedaStarField::~LocalAndromedaStarField()
{
    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void LocalAndromedaStarField::Render() const
{
    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_POINTS, 0, CandidatesPerChunk, ChunkCount);
    glBindVertexArray(0);
}

std::size_t LocalAndromedaStarField::GetCandidateCount() const
{
    return static_cast<std::size_t>(CandidatesPerChunk) * static_cast<std::size_t>(ChunkCount);
}

double LocalAndromedaStarField::GetRangeParsecs() const
{
    return 300.0;
}
