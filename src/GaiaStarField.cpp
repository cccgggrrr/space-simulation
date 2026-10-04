#include "GaiaStarField.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include "GalacticConstants.h"

namespace
{
    constexpr double Pi = 3.14159265358979323846;
    constexpr double DegToRad = Pi / 180.0;
}

GaiaStarField::GaiaStarField(const std::string& csvPath)
{
    Load(csvPath);
    CreateBuffers();
}

GaiaStarField::~GaiaStarField()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

std::vector<std::string> GaiaStarField::ParseCsvLine(const std::string& line)
{
    std::vector<std::string> fields;
    std::string field;
    bool quoted = false;

    for (std::size_t i = 0; i < line.size(); i++)
    {
        char c = line[i];

        if (c == '"')
        {
            if (quoted && i + 1 < line.size() && line[i + 1] == '"')
            {
                field.push_back('"');
                i++;
            }
            else
                quoted = !quoted;
        }
        else if (c == ',' && !quoted)
        {
            fields.push_back(field);
            field.clear();
        }
        else
            field.push_back(c);
    }

    fields.push_back(field);
    return fields;
}

glm::vec3 GaiaStarField::BpRpToColor(double bpRp)
{
    double t = std::clamp((bpRp + 0.5) / 4.0, 0.0, 1.0);
    glm::dvec3 blue(0.62, 0.74, 1.0);
    glm::dvec3 white(1.0, 0.97, 0.90);
    glm::dvec3 orange(1.0, 0.55, 0.28);
    glm::dvec3 color;

    if (t < 0.45)
        color = glm::mix(blue, white, t / 0.45);
    else
        color = glm::mix(white, orange, (t - 0.45) / 0.55);

    return glm::vec3(color);
}

void GaiaStarField::Load(const std::string& csvPath)
{
    std::ifstream file(csvPath);

    if (!file)
        throw std::runtime_error("Could not open Gaia CSV: " + csvPath);

    std::string line;

    if (!std::getline(file, line))
        throw std::runtime_error("Gaia CSV is empty: " + csvPath);

    std::vector<std::string> header = ParseCsvLine(line);
    std::unordered_map<std::string, std::size_t> columns;

    for (std::size_t i = 0; i < header.size(); i++)
        columns[header[i]] = i;

    const char* required[] = { "source_id", "l", "b", "parallax", "parallax_over_error", "phot_g_mean_mag", "bp_rp" };

    for (const char* name : required)
    {
        if (!columns.contains(name))
            throw std::runtime_error(std::string("Missing Gaia CSV column: ") + name);
    }

    while (std::getline(file, line))
    {
        if (line.empty())
            continue;

        std::vector<std::string> fields = ParseCsvLine(line);

        if (fields.size() < header.size())
            continue;

        try
        {
            double parallax = std::stod(fields[columns["parallax"]]);
            double parallaxOverError = std::stod(fields[columns["parallax_over_error"]]);
            double apparentMagnitude = std::stod(fields[columns["phot_g_mean_mag"]]);
            double bpRp = std::stod(fields[columns["bp_rp"]]);

            if (parallax <= 0.0 || parallaxOverError < 5.0)
                continue;

            double l = std::stod(fields[columns["l"]]) * DegToRad;
            double b = std::stod(fields[columns["b"]]) * DegToRad;
            double distanceParsec = 1000.0 / parallax;
            double distanceMeters = distanceParsec * GalacticConstants::Parsec;
            double cosB = std::cos(b);

            Star star;
            star.SourceId = static_cast<std::uint64_t>(std::stoull(fields[columns["source_id"]]));
            star.Position = glm::dvec3(
                distanceMeters * cosB * std::cos(l),
                distanceMeters * cosB * std::sin(l),
                distanceMeters * std::sin(b)
            );
            star.Color = BpRpToColor(bpRp);
            star.AbsoluteMagnitude = apparentMagnitude - 5.0 * std::log10(distanceParsec) + 5.0;
            stars.push_back(star);
        }
        catch (...)
        {
        }
    }

    renderStars.resize(stars.size());
}

void GaiaStarField::CreateBuffers()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, renderStars.size() * sizeof(RenderStar), nullptr, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(RenderStar), reinterpret_cast<void*>(offsetof(RenderStar, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(RenderStar), reinterpret_cast<void*>(offsetof(RenderStar, Color)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(RenderStar), reinterpret_cast<void*>(offsetof(RenderStar, Magnitude)));

    glBindVertexArray(0);
}

void GaiaStarField::Render(const glm::dvec3& cameraPosition)
{
    for (std::size_t i = 0; i < stars.size(); i++)
    {
        glm::dvec3 relativeMeters = stars[i].Position - cameraPosition;
        glm::dvec3 relativeParsec = relativeMeters / GalacticConstants::Parsec;
        double distanceParsec = std::max(glm::length(relativeParsec), 1.0e-6);
        double apparentMagnitude = stars[i].AbsoluteMagnitude + 5.0 * std::log10(distanceParsec) - 5.0;

        renderStars[i].Position = glm::vec3(relativeParsec);
        renderStars[i].Color = stars[i].Color;
        renderStars[i].Magnitude = static_cast<float>(apparentMagnitude);
    }

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferSubData(GL_ARRAY_BUFFER, 0, renderStars.size() * sizeof(RenderStar), renderStars.data());

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(renderStars.size()));
    glBindVertexArray(0);
}

std::size_t GaiaStarField::GetStarCount() const
{
    return stars.size();
}
