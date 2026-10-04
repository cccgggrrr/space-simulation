#pragma once

#include <cstddef>
#include <GL/glew.h>

class LocalAndromedaStarField
{
public:
    LocalAndromedaStarField();
    ~LocalAndromedaStarField();

    LocalAndromedaStarField(const LocalAndromedaStarField&) = delete;
    LocalAndromedaStarField& operator=(const LocalAndromedaStarField&) = delete;

    void Render() const;

    std::size_t GetCandidateCount() const;
    double GetRangeParsecs() const;

private:
    GLuint vao = 0;

    static constexpr GLsizei CandidatesPerChunk = 160;
    static constexpr GLsizei GridX = 9;
    static constexpr GLsizei GridY = 9;
    static constexpr GLsizei GridZ = 7;
    static constexpr GLsizei ChunkCount = GridX * GridY * GridZ;
};
