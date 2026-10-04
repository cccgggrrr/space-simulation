#include "MilkyWayDensityRenderer.h"

#include <algorithm>
#include <cmath>
#include <random>

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

    float RandomBrightness(std::mt19937_64& rng, float minimum, float maximum)
    {
        double u = Uniform01(rng);
        double shaped = std::pow(u, 1.65);
        return minimum + static_cast<float>(shaped) * (maximum - minimum);
    }

    double LayerField(double radius, double angle, double z, double radialA, double angularA, double zA)
    {
        double field = 0.52
            + 0.24 * std::sin(radius / radialA + angle * angularA + z / zA)
            + 0.16 * std::sin(radius / (radialA * 0.48) - angle * (angularA * 1.7) + z / (zA * 0.7))
            + 0.08 * std::sin(radius / (radialA * 0.22) + angle * (angularA * 2.9) - z / (zA * 0.45));
        return std::clamp(field, 0.0, 1.0);
    }

    glm::vec3 SmoothCloudColor(double x, double y, double z, double diskRadius)
    {
        double radius = std::sqrt(x * x + y * y);
        double angle = std::atan2(y, x);
        double arm = MilkyWayModel::ArmDensity(radius, angle);
        double core = MilkyWayModel::CoreDensity(x, y, z);
        double bulge = MilkyWayModel::BulgeDensity(x, y, z);
        float radial = static_cast<float>(std::clamp(radius / diskRadius, 0.0, 1.0));

        double field = 0.62 * std::sin(radius / 1550.0 + angle * 1.45)
            + 0.36 * std::sin(radius / 760.0 - angle * 1.95)
            + 0.17 * std::sin(radius / 350.0 + angle * 2.85);

        double emissionField = LayerField(radius, angle, z, 1850.0, 1.15, 620.0);
        double reflectionField = LayerField(radius, angle, z, 2300.0, -0.95, 900.0);
        double veilField = LayerField(radius, angle, z, 2900.0, 0.68, 1300.0);

        float chroma = static_cast<float>(0.5 + 0.5 * std::tanh(field * 0.48));
        glm::vec3 blue(0.38f, 0.62f, 1.00f);
        glm::vec3 paleBlue(0.72f, 0.84f, 1.00f);
        glm::vec3 palePink(1.00f, 0.62f, 0.80f);
        glm::vec3 lavender(0.80f, 0.72f, 1.00f);
        glm::vec3 neutral(0.91f, 0.93f, 1.00f);
        glm::vec3 warm(1.00f, 0.72f, 0.36f);
        glm::vec3 hot(1.00f, 0.97f, 0.78f);
        glm::vec3 reflectionBlue(0.70f, 0.84f, 1.00f);
        glm::vec3 reflectionTeal(0.80f, 0.92f, 1.00f);
        glm::vec3 emissionPink(1.00f, 0.58f, 0.74f);
        glm::vec3 emissionRose(0.98f, 0.44f, 0.62f);
        glm::vec3 emissionLavender(0.92f, 0.70f, 1.00f);

        glm::vec3 cool = glm::mix(blue, paleBlue, 0.58f);
        glm::vec3 warmArm = glm::mix(palePink, lavender, 0.24f);
        glm::vec3 armColor = glm::mix(cool, warmArm, chroma);
        float neutralMix = static_cast<float>(0.18 + 0.30 * (1.0 - std::pow(arm, 0.40)));
        glm::vec3 diskColor = glm::mix(armColor, neutral, neutralMix);

        float bulgeMix = static_cast<float>(MilkyWayModel::Smooth01(bulge));
        glm::vec3 bulgeColor = glm::mix(glm::vec3(1.00f, 0.82f, 0.62f), warm, 0.38f);
        glm::vec3 color = glm::mix(diskColor, bulgeColor, bulgeMix * 0.74f);

        float coreMix = static_cast<float>(MilkyWayModel::Smooth01(core));
        color = glm::mix(color, hot, coreMix * 0.88f);

        float emissionMask = static_cast<float>(std::pow(std::clamp(0.66 * arm + 0.34 * emissionField - 0.56, 0.0, 1.0), 1.45));
        float reflectionMask = static_cast<float>(std::pow(std::clamp(0.50 * arm + 0.50 * reflectionField - 0.54, 0.0, 1.0), 1.30));
        float veilMask = static_cast<float>(std::pow(std::clamp(0.40 * (1.0 - arm) + 0.60 * veilField - 0.48, 0.0, 1.0), 1.10));

        glm::vec3 emissionColor = glm::mix(emissionPink, emissionRose, static_cast<float>(0.5 + 0.5 * std::sin(radius / 1200.0 - angle * 1.8)));
        emissionColor = glm::mix(emissionColor, emissionLavender, static_cast<float>(0.22 + 0.25 * std::sin(radius / 2400.0 + angle * 0.9)));
        glm::vec3 reflectionColor = glm::mix(reflectionBlue, reflectionTeal, static_cast<float>(0.5 + 0.5 * std::sin(radius / 1750.0 + angle * 1.1)));
        glm::vec3 veilColor = glm::mix(glm::vec3(0.92f, 0.84f, 1.00f), glm::vec3(0.86f, 0.78f, 0.98f), static_cast<float>(0.5 + 0.5 * std::sin(radius / 2100.0 - angle * 0.8)));

        color = glm::mix(color, reflectionColor, reflectionMask * (0.26f + 0.10f * (1.0f - coreMix)));
        color = glm::mix(color, emissionColor, emissionMask * (0.34f + 0.10f * (1.0f - coreMix)));
        color = glm::mix(color, veilColor, veilMask * (0.16f + 0.16f * (1.0f - radial)));

        double radialWhite = 0.10 + 0.20 * (1.0 - std::clamp(radius / diskRadius, 0.0, 1.0));
        color = glm::mix(color, neutral, static_cast<float>(radialWhite));
        return glm::clamp(color, glm::vec3(0.0f), glm::vec3(1.0f));
    }

    glm::vec3 BaseStellarColor(double x, double y, double z, double diskRadius)
    {
        double radius = std::sqrt(x * x + y * y);
        double angle = std::atan2(y, x);
        double arm = MilkyWayModel::ArmDensity(radius, angle);
        double core = MilkyWayModel::CoreDensity(x, y, z);
        double bulge = MilkyWayModel::BulgeDensity(x, y, z);
        float radial = static_cast<float>(std::clamp(radius / diskRadius, 0.0, 1.0));

        glm::vec3 outer = glm::mix(glm::vec3(0.90f, 0.91f, 0.97f), glm::vec3(0.70f, 0.82f, 1.00f), radial * 0.70f);
        glm::vec3 armTint = glm::mix(glm::vec3(0.78f, 0.88f, 1.00f), glm::vec3(1.00f, 0.76f, 0.88f), static_cast<float>(0.5 + 0.5 * std::sin(radius / 1250.0 + angle * 1.7)));
        glm::vec3 color = glm::mix(outer, armTint, static_cast<float>(0.34 * std::pow(arm, 0.45)));
        color = glm::mix(color, glm::vec3(1.00f, 0.76f, 0.55f), static_cast<float>(0.62 * MilkyWayModel::Smooth01(bulge)));
        color = glm::mix(color, glm::vec3(1.00f, 0.96f, 0.78f), static_cast<float>(0.72 * MilkyWayModel::Smooth01(core)));
        return color;
    }
}

MilkyWayDensityRenderer::MilkyWayDensityRenderer(std::size_t particleCount)
{
    Build(particleCount);
    CreateBuffers();
}

MilkyWayDensityRenderer::~MilkyWayDensityRenderer()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void MilkyWayDensityRenderer::Build(std::size_t particleCount)
{
    particles.clear();
    particles.reserve(particleCount);

    std::size_t baseCount = static_cast<std::size_t>(particleCount * 0.30);
    std::size_t cloudCount = static_cast<std::size_t>(particleCount * 0.62);
    std::size_t dustCount = particleCount - baseCount - cloudCount;

    std::mt19937_64 rng(0x4D494C4B59574159ull);
    double diskRadius = GalacticConstants::MilkyWayDiskRadiusParsec;

    for (std::size_t i = 0; i < baseCount; i++)
    {
        double radius;
        double angle;
        double z;

        if (Uniform01(rng) < 0.22)
        {
            std::normal_distribution<double> xDist(0.0, 2100.0);
            std::normal_distribution<double> yDist(0.0, 900.0);
            std::normal_distribution<double> zDist(0.0, 720.0);
            double x0 = xDist(rng);
            double y0 = yDist(rng);
            double c = std::cos(MilkyWayModel::BarAngle);
            double s = std::sin(MilkyWayModel::BarAngle);
            double x = x0 * c - y0 * s;
            double y = x0 * s + y0 * c;
            z = zDist(rng);
            radius = std::sqrt(x * x + y * y);
            angle = std::atan2(y, x);

            if (radius >= diskRadius)
            {
                i--;
                continue;
            }
        }
        else
        {
            radius = SampleDiskRadius(rng, 4400.0, 400.0, diskRadius - 100.0);
            angle = Uniform01(rng) * MilkyWayModel::TwoPi;
            double height = MilkyWayModel::DiskScaleHeight(radius);
            z = SampleSignedExponential(rng, height * 0.72);
        }

        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        double arm = MilkyWayModel::ArmDensity(radius, angle);
        double edge = MilkyWayModel::DiskEdgeTaper(radius, diskRadius);
        double outer = MilkyWayModel::Smooth01((radius - 6000.0) / 8300.0);
        double baseline = 0.20 * (1.0 - outer) + 0.045 * outer;
        double interArm = 0.07 * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.82));
        double keepProbability = std::clamp((baseline + interArm + 0.67 * std::pow(arm, 0.42) + 0.53 * MilkyWayModel::BulgeDensity(x, y, z)) * edge, 0.0, 1.0);

        if (Uniform01(rng) > keepProbability)
        {
            i--;
            continue;
        }

        Particle particle;
        particle.Position = glm::vec3(x, y, z);
        particle.Color = BaseStellarColor(x, y, z, diskRadius);
        particle.Brightness = RandomBrightness(rng, 0.018f, 0.075f) * static_cast<float>(0.70 + 0.45 * std::pow(arm, 0.40) + 0.35 * MilkyWayModel::CoreDensity(x, y, z));
        particle.Size = 0.95f + static_cast<float>(Uniform01(rng)) * 0.88f;
        particle.Mode = 0.0f;
        particle.Seed = static_cast<float>(Uniform01(rng));
        particles.push_back(particle);
    }

    baseParticleCount = particles.size();

    for (std::size_t i = 0; i < cloudCount; i++)
    {
        double radius;
        double angle;
        double z;

        if (Uniform01(rng) < 0.20)
        {
            std::normal_distribution<double> xDist(0.0, 2300.0);
            std::normal_distribution<double> yDist(0.0, 1050.0);
            std::normal_distribution<double> zDist(0.0, 680.0);
            double x0 = xDist(rng);
            double y0 = yDist(rng);
            double c = std::cos(MilkyWayModel::BarAngle);
            double s = std::sin(MilkyWayModel::BarAngle);
            double x = x0 * c - y0 * s;
            double y = x0 * s + y0 * c;
            z = zDist(rng);
            radius = std::sqrt(x * x + y * y);
            angle = std::atan2(y, x);
        }
        else
        {
            radius = SampleDiskRadius(rng, 4700.0, 1000.0, diskRadius - 120.0);
            angle = Uniform01(rng) * MilkyWayModel::TwoPi;
            double height = MilkyWayModel::DiskScaleHeight(radius);
            z = SampleSignedExponential(rng, height * 0.88);
        }

        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        double density = MilkyWayModel::CloudDensity(x, y, z, diskRadius);

        if (Uniform01(rng) > density)
        {
            i--;
            continue;
        }

        double arm = MilkyWayModel::ArmDensity(radius, angle);
        double core = MilkyWayModel::CoreDensity(x, y, z);
        double bulge = MilkyWayModel::BulgeDensity(x, y, z);
        double cloudWeight = 0.56 + 0.40 * std::pow(arm, 0.50) + 0.16 * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.72)) + 0.38 * bulge + 0.38 * core;

        Particle particle;
        particle.Position = glm::vec3(x, y, z);
        particle.Color = SmoothCloudColor(x, y, z, diskRadius);
        particle.Brightness = RandomBrightness(rng, 0.028f, 0.098f) * static_cast<float>(std::clamp(cloudWeight, 0.0, 1.38));
        particle.Size = 6.0f + static_cast<float>(Uniform01(rng)) * 13.0f;
        particle.Mode = 1.0f;
        particle.Seed = static_cast<float>(Uniform01(rng));
        particles.push_back(particle);
    }

    cloudParticleCount = particles.size() - baseParticleCount;

    for (std::size_t i = 0; i < dustCount; i++)
    {
        double radius = SampleDiskRadius(rng, 5000.0, 1900.0, diskRadius - 380.0);
        double angle = Uniform01(rng) * MilkyWayModel::TwoPi;
        double arm = MilkyWayModel::ArmDensity(radius, angle);
        double edge = MilkyWayModel::DiskEdgeTaper(radius, diskRadius);

        double patchField = 0.50
            + 0.28 * std::sin(radius / 620.0 + angle * 2.7)
            + 0.16 * std::sin(radius / 255.0 - angle * 5.1)
            + 0.08 * std::sin(radius / 110.0 + angle * 8.4);
        patchField = std::clamp(patchField, 0.0, 1.0);

        double armWeight = std::pow(arm, 0.72);
        double keepProbability = (0.012 + 0.46 * armWeight) * (0.20 + 0.80 * patchField) * edge;

        if (Uniform01(rng) > keepProbability)
        {
            i--;
            continue;
        }

        double height = MilkyWayModel::DiskScaleHeight(radius);
        double z = SampleSignedExponential(rng, height * (0.58 + 0.34 * Uniform01(rng)));

        double radialJitter = (Uniform01(rng) - 0.5) * (180.0 + 420.0 * Uniform01(rng));
        double tangentialJitter = (Uniform01(rng) - 0.5) * (260.0 + 620.0 * Uniform01(rng));

        double cosA = std::cos(angle);
        double sinA = std::sin(angle);
        double x = radius * cosA + radialJitter * cosA - tangentialJitter * sinA;
        double y = radius * sinA + radialJitter * sinA + tangentialJitter * cosA;

        float dustHue = static_cast<float>(0.5 + 0.5 * std::sin(radius / 880.0 - angle * 2.35 + z / 160.0));
        glm::vec3 dustCool(0.018f, 0.022f, 0.034f);
        glm::vec3 dustMauve(0.030f, 0.024f, 0.041f);
        glm::vec3 dustWarm(0.035f, 0.028f, 0.022f);

        Particle particle;
        particle.Position = glm::vec3(x, y, z);
        particle.Color = glm::mix(glm::mix(dustCool, dustMauve, dustHue), dustWarm, static_cast<float>(0.16 + 0.24 * patchField));
        particle.Brightness = RandomBrightness(rng, 0.006f, 0.024f) * static_cast<float>((0.45 + 0.35 * armWeight) * (0.55 + 0.45 * patchField));
        particle.Size = 4.0f + static_cast<float>(Uniform01(rng)) * 8.5f;
        particle.Mode = 2.0f;
        particle.Seed = static_cast<float>(Uniform01(rng));
        particles.push_back(particle);
    }

    dustParticleCount = particles.size() - baseParticleCount - cloudParticleCount;
}

void MilkyWayDensityRenderer::CreateBuffers()
{
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, particles.size() * sizeof(Particle), particles.data(), GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void*>(offsetof(Particle, Position)));

    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void*>(offsetof(Particle, Color)));

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void*>(offsetof(Particle, Brightness)));

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void*>(offsetof(Particle, Size)));

    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void*>(offsetof(Particle, Mode)));

    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Particle), reinterpret_cast<void*>(offsetof(Particle, Seed)));

    glBindVertexArray(0);
}

void MilkyWayDensityRenderer::Render() const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(baseParticleCount));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void MilkyWayDensityRenderer::RenderClouds() const
{
    if (cloudParticleCount == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, static_cast<GLint>(baseParticleCount), static_cast<GLsizei>(cloudParticleCount));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void MilkyWayDensityRenderer::RenderDust() const
{
    if (dustParticleCount == 0)
        return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, static_cast<GLint>(baseParticleCount + cloudParticleCount), static_cast<GLsizei>(dustParticleCount));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

std::size_t MilkyWayDensityRenderer::GetParticleCount() const
{
    return particles.size();
}
