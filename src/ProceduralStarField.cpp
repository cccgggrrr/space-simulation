#include "ProceduralStarField.h"

#include <algorithm>
#include <cmath>
#include <random>
#include <utility>

#include "GalacticConstants.h"
#include "MilkyWayModel.h"

namespace
{
    double Uniform01(std::mt19937_64& rng)
    {
        static std::uniform_real_distribution<double> distribution(0.0, 1.0);
        return std::clamp(distribution(rng), 1.0e-12, 1.0 - 1.0e-12);
    }

    double SampleExponential(std::mt19937_64& rng, double scale)
    {
        return -scale * std::log(Uniform01(rng));
    }

    double SampleSignedExponential(std::mt19937_64& rng, double scale)
    {
        double value = SampleExponential(rng, scale);
        return Uniform01(rng) < 0.5 ? -value : value;
    }

    double SampleDiskRadius(std::mt19937_64& rng, double scaleLength, double minimumRadius, double maximumRadius)
    {
        for (;;)
        {
            double radius = -scaleLength * std::log(Uniform01(rng) * Uniform01(rng));

            if (radius >= minimumRadius && radius <= maximumRadius)
                return radius;
        }
    }

    double SampleAreaRadius(std::mt19937_64& rng, double minimumRadius, double maximumRadius)
    {
        double min2 = minimumRadius * minimumRadius;
        double max2 = maximumRadius * maximumRadius;
        return std::sqrt(min2 + Uniform01(rng) * (max2 - min2));
    }

    glm::vec3 ClampColor(const glm::dvec3& color)
    {
        return glm::vec3(
            static_cast<float>(std::clamp(color.x, 0.0, 1.0)),
            static_cast<float>(std::clamp(color.y, 0.0, 1.0)),
            static_cast<float>(std::clamp(color.z, 0.0, 1.0))
        );
    }

    glm::vec3 TemperatureToColor(double kelvin)
    {
        kelvin = std::clamp(kelvin, 2400.0, 40000.0) / 100.0;
        double red;
        double green;
        double blue;

        if (kelvin <= 66.0)
        {
            red = 255.0;
            green = 99.4708025861 * std::log(kelvin) - 161.1195681661;
            blue = kelvin <= 19.0 ? 0.0 : 138.5177312231 * std::log(kelvin - 10.0) - 305.0447927307;
        }
        else
        {
            double t = kelvin - 60.0;
            red = 329.698727446 * std::pow(t, -0.1332047592);
            green = 288.1221695283 * std::pow(t, -0.0755148492);
            blue = 255.0;
        }

        return ClampColor(glm::dvec3(red, green, blue) / 255.0);
    }

    void SampleStellarProperties(std::mt19937_64& rng, glm::vec3& color, float& absoluteMagnitude)
    {
        double u = Uniform01(rng);

        if (u < 0.40)
        {
            double temperature = 2800.0 + Uniform01(rng) * 2600.0;
            color = TemperatureToColor(temperature);
            absoluteMagnitude = static_cast<float>(4.7 + Uniform01(rng) * 4.8);
        }
        else if (u < 0.72)
        {
            double temperature = 5000.0 + Uniform01(rng) * 3300.0;
            color = TemperatureToColor(temperature);
            absoluteMagnitude = static_cast<float>(0.8 + Uniform01(rng) * 4.5);
        }
        else if (u < 0.93)
        {
            double temperature = 3600.0 + Uniform01(rng) * 3200.0;
            color = TemperatureToColor(temperature);
            absoluteMagnitude = static_cast<float>(-2.8 + Uniform01(rng) * 4.8);
        }
        else
        {
            double temperature = 9000.0 + Uniform01(rng) * 25000.0;
            color = TemperatureToColor(temperature);
            absoluteMagnitude = static_cast<float>(-6.2 + Uniform01(rng) * 5.2);
        }
    }

    const MilkyWayModel::Arm& PickArm(std::mt19937_64& rng)
    {
        double total = 0.0;

        for (const MilkyWayModel::Arm& arm : MilkyWayModel::Arms)
            total += arm.Strength;

        double value = Uniform01(rng) * total;

        for (const MilkyWayModel::Arm& arm : MilkyWayModel::Arms)
        {
            value -= arm.Strength;

            if (value <= 0.0)
                return arm;
        }

        return MilkyWayModel::Arms.back();
    }

    glm::dvec3 SampleArmStar(std::mt19937_64& rng)
    {
        for (;;)
        {
            const MilkyWayModel::Arm& arm = PickArm(rng);
            double radius = SampleDiskRadius(rng, 4500.0, arm.MinRadiusParsec, arm.MaxRadiusParsec);

            if (Uniform01(rng) > MilkyWayModel::ArmSegmentStrength(arm, radius))
                continue;

            double width = MilkyWayModel::ArmWidthParsec(arm, radius);
            double spread = Uniform01(rng);
            double sigma = spread < 0.48 ? width * 0.66 : spread < 0.84 ? width * 1.30 : width * 2.20;
            std::normal_distribution<double> acrossArm(0.0, sigma);
            double offset = acrossArm(rng);
            radius = std::clamp(radius + offset * 0.12, arm.MinRadiusParsec, arm.MaxRadiusParsec);
            double angle = MilkyWayModel::ArmAngle(arm, radius) + offset / std::max(radius, 1.0);
            double z = SampleSignedExponential(rng, 180.0 + radius * 0.0060);
            return glm::dvec3(radius * std::cos(angle), radius * std::sin(angle), z);
        }
    }

    std::uint32_t PackColor(const glm::vec3& color)
    {
        std::uint32_t r = static_cast<std::uint32_t>(std::clamp(color.r, 0.0f, 1.0f) * 255.0f + 0.5f);
        std::uint32_t g = static_cast<std::uint32_t>(std::clamp(color.g, 0.0f, 1.0f) * 255.0f + 0.5f);
        std::uint32_t b = static_cast<std::uint32_t>(std::clamp(color.b, 0.0f, 1.0f) * 255.0f + 0.5f);
        return r | (g << 8) | (b << 16) | (255u << 24);
    }

    constexpr double PositionLowScale = 0.002;

    std::int16_t QuantizeMagnitude(float magnitude)
    {
        double value = std::round(static_cast<double>(magnitude) * 100.0);
        return static_cast<std::int16_t>(std::clamp(value, -32768.0, 32767.0));
    }

    std::int16_t QuantizePositionLow(double value)
    {
        double normalized = std::clamp(value / PositionLowScale, -1.0, 1.0);
        return static_cast<std::int16_t>(std::round(normalized * 32767.0));
    }

    void StoreSplitPosition(const glm::dvec3& position, float high[3], std::int16_t lowQ[3])
    {
        glm::vec3 positionHigh = glm::vec3(position);
        glm::dvec3 residual = position - glm::dvec3(positionHigh);

        high[0] = positionHigh.x;
        high[1] = positionHigh.y;
        high[2] = positionHigh.z;

        lowQ[0] = QuantizePositionLow(residual.x);
        lowQ[1] = QuantizePositionLow(residual.y);
        lowQ[2] = QuantizePositionLow(residual.z);
    }
}

ProceduralStarField::ProceduralStarField(std::size_t requestedStarCount)
{
    Generate(requestedStarCount);
    CreateBuffers();
}

ProceduralStarField::~ProceduralStarField()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void ProceduralStarField::Generate(std::size_t requestedStarCount)
{
    stars.clear();
    stars.reserve(requestedStarCount);

    std::mt19937_64 rng(0x50524F4353544152ull);
    std::normal_distribution<double> bulgeX(0.0, 1500.0);
    std::normal_distribution<double> bulgeY(0.0, 650.0);
    std::normal_distribution<double> bulgeZ(0.0, 650.0);

    double diskRadius = GalacticConstants::MilkyWayDiskRadiusParsec;
    double cosBar = std::cos(MilkyWayModel::BarAngle);
    double sinBar = std::sin(MilkyWayModel::BarAngle);

    for (std::size_t i = 0; i < requestedStarCount; i++)
    {
        glm::dvec3 position;
        double component = Uniform01(rng);

        if (component < 0.42)
        {
            double radius = SampleDiskRadius(rng, 4550.0, 500.0, diskRadius - 60.0);
            double angle = Uniform01(rng) * MilkyWayModel::TwoPi;
            double armDensity = MilkyWayModel::ArmDensity(radius, angle);
            double transition = std::pow(armDensity, 0.34);
            double outerT = std::clamp((radius - 6200.0) / 8200.0, 0.0, 1.0);
            outerT = outerT * outerT * (3.0 - 2.0 * outerT);
            double baseline = 0.13 * (1.0 - outerT) + 0.012 * outerT;
            double armBoost = 0.70 * (1.0 - outerT) + 0.88 * outerT;
            double keepProbability = std::clamp(baseline + armBoost * transition, 0.0, 1.0);

            if (Uniform01(rng) > keepProbability)
            {
                i--;
                continue;
            }

            double z = SampleSignedExponential(rng, 240.0 + radius * 0.0080);
            position = glm::dvec3(radius * std::cos(angle), radius * std::sin(angle), z);
        }
        else if (component < 0.82)
            position = SampleArmStar(rng);
        else if (component < 0.87)
        {
            double radius = SampleAreaRadius(rng, 8000.0, diskRadius - 35.0);
            double angle = Uniform01(rng) * MilkyWayModel::TwoPi;
            double armDensity = MilkyWayModel::ArmDensity(radius, angle);
            double transition = std::pow(armDensity, 0.32);
            double outerT = std::clamp((radius - 8000.0) / 6900.0, 0.0, 1.0);
            outerT = outerT * outerT * (3.0 - 2.0 * outerT);
            double keepProbability = (0.012 + 0.72 * transition) * (1.0 - 0.50 * outerT);

            if (Uniform01(rng) > keepProbability)
            {
                i--;
                continue;
            }

            double z = SampleSignedExponential(rng, 330.0 + radius * 0.0105);
            position = glm::dvec3(radius * std::cos(angle), radius * std::sin(angle), z);
        }
        else if (component < 0.98)
        {
            double x0 = bulgeX(rng);
            double y0 = bulgeY(rng);
            double z = bulgeZ(rng);
            double x = x0 * cosBar - y0 * sinBar;
            double y = x0 * sinBar + y0 * cosBar;
            position = glm::dvec3(x, y, z);
        }
        else
        {
            double radius = SampleDiskRadius(rng, 5200.0, 1500.0, diskRadius - 120.0);
            double angle = Uniform01(rng) * MilkyWayModel::TwoPi;
            double z = SampleSignedExponential(rng, 1050.0);
            position = glm::dvec3(radius * std::cos(angle), radius * std::sin(angle), z);
        }

        double cylindricalRadius = std::sqrt(position.x * position.x + position.y * position.y);

        if (cylindricalRadius >= diskRadius)
        {
            i--;
            continue;
        }

        glm::vec3 color;
        float absoluteMagnitude;
        SampleStellarProperties(rng, color, absoluteMagnitude);

        StarVertex star{};
        StoreSplitPosition(position, star.PositionHigh, star.PositionLowQ);
        star.PackedColor = PackColor(color);
        star.MagnitudeQ = QuantizeMagnitude(absoluteMagnitude);
        stars.push_back(star);
    }
}

void ProceduralStarField::CreateBuffers()
{
    starCount = stars.size();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, stars.size() * sizeof(StarVertex), stars.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, PositionHigh)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, PackedColor)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_SHORT, GL_FALSE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, MagnitudeQ)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_SHORT, GL_TRUE, sizeof(StarVertex), reinterpret_cast<void*>(offsetof(StarVertex, PositionLowQ)));

    glBindVertexArray(0);

    std::vector<StarVertex>().swap(stars);
}

void ProceduralStarField::Render() const
{
    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(starCount));
    glBindVertexArray(0);
}

std::size_t ProceduralStarField::GetStarCount() const
{
    return starCount;
}
