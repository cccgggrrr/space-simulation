#pragma once

#include <cstddef>
#include <GL/glew.h>

class LocalMilkyWayStarField
{
public:
    LocalMilkyWayStarField();
    ~LocalMilkyWayStarField();

    LocalMilkyWayStarField(const LocalMilkyWayStarField&) = delete;
    LocalMilkyWayStarField& operator=(const LocalMilkyWayStarField&) = delete;

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
