#include "AndromedaDensityRenderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>

#include "AndromedaModel.h"

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
        double shaped = std::pow(u, 1.58);
        return minimum + static_cast<float>(shaped) * (maximum - minimum);
    }

    double LayerField(double radius, double angle, double z, double radialA, double angularA, double zA)
    {
        double field = 0.52
            + 0.22 * std::sin(radius / radialA + angle * angularA + z / zA)
            + 0.13 * std::sin(radius / (radialA * 0.52) - angle * (angularA * 1.65) + z / (zA * 0.72))
            + 0.07 * std::sin(radius / (radialA * 0.24) + angle * (angularA * 2.6) - z / (zA * 0.48));
        return std::clamp(field, 0.0, 1.0);
    }

    glm::vec3 TransformToWorld(double x, double y, double z)
    {
        glm::dvec3 local(x, y, z);
        glm::dvec3 world = AndromedaModel::Orientation() * local + AndromedaModel::CenterParsecs();
        return glm::vec3(world);
    }

    glm::vec3 CloudColor(double x, double y, double z)
    {
        double radius = std::sqrt(x * x + y * y);
        double angle = std::atan2(y, x);
        double arm = AndromedaModel::ArmDensity(radius, angle);
        double bulge = AndromedaModel::BulgeDensity(x, y, z);
        double core = AndromedaModel::CoreDensity(x, y, z);

        double field = 0.58 * std::sin(radius / 2200.0 + angle * 1.15)
            + 0.28 * std::sin(radius / 870.0 - angle * 1.65)
            + 0.12 * std::sin(radius / 360.0 + angle * 2.45);
        double reflectionField = LayerField(radius, angle, z, 3000.0, 0.78, 1200.0);
        double emissionField = LayerField(radius, angle, z, 2200.0, -1.05, 720.0);
        double veilField = LayerField(radius, angle, z, 3600.0, 0.55, 1500.0);

        float chroma = static_cast<float>(0.5 + 0.5 * std::tanh(field * 0.42));
        glm::vec3 paleBlue(0.76f, 0.87f, 1.00f);
        glm::vec3 blue(0.60f, 0.74f, 1.00f);
        glm::vec3 palePink(1.00f, 0.76f, 0.86f);
        glm::vec3 white(0.96f, 0.97f, 1.00f);
        glm::vec3 warm(1.00f, 0.84f, 0.68f);
        glm::vec3 hot(1.00f, 0.97f, 0.82f);
        glm::vec3 coolReflection(0.80f, 0.90f, 1.00f);
        glm::vec3 blueReflection(0.68f, 0.80f, 1.00f);
        glm::vec3 pinkEmission(1.00f, 0.72f, 0.84f);
        glm::vec3 roseEmission(1.00f, 0.60f, 0.74f);
        glm::vec3 veilWhite(0.96f, 0.94f, 1.00f);

        glm::vec3 armCool = glm::mix(blue, paleBlue, 0.58f);
        glm::vec3 armWarm = glm::mix(palePink, white, 0.36f);
        glm::vec3 diskColor = glm::mix(armCool, armWarm, chroma);
        diskColor = glm::mix(diskColor, white, static_cast<float>(0.18 + 0.16 * (1.0 - std::pow(arm, 0.55))));
        diskColor = glm::mix(diskColor, warm, static_cast<float>(0.56 * AndromedaModel::Smooth01(bulge)));
        diskColor = glm::mix(diskColor, hot, static_cast<float>(0.80 * AndromedaModel::Smooth01(core)));

        float reflectionMask = static_cast<float>(std::pow(std::clamp(0.44 * arm + 0.56 * reflectionField - 0.56, 0.0, 1.0), 1.20));
        float emissionMask = static_cast<float>(std::pow(std::clamp(0.58 * arm + 0.42 * emissionField - 0.60, 0.0, 1.0), 1.25));
        float veilMask = static_cast<float>(std::pow(std::clamp(0.32 * (1.0 - arm) + 0.68 * veilField - 0.50, 0.0, 1.0), 1.05));

        glm::vec3 reflectionColor = glm::mix(blueReflection, coolReflection, static_cast<float>(0.5 + 0.5 * std::sin(radius / 2600.0 + angle * 0.8)));
        glm::vec3 emissionColor = glm::mix(pinkEmission, roseEmission, static_cast<float>(0.5 + 0.5 * std::sin(radius / 1700.0 - angle * 1.4)));
        diskColor = glm::mix(diskColor, reflectionColor, reflectionMask * 0.24f);
        diskColor = glm::mix(diskColor, emissionColor, emissionMask * 0.22f);
        diskColor = glm::mix(diskColor, veilWhite, veilMask * 0.12f);

        return glm::clamp(diskColor, glm::vec3(0.0f), glm::vec3(1.0f));
    }

    glm::vec3 StellarColor(double x, double y, double z)
    {
        double radius = std::sqrt(x * x + y * y);
        double angle = std::atan2(y, x);
        double arm = AndromedaModel::ArmDensity(radius, angle);
        double bulge = AndromedaModel::BulgeDensity(x, y, z);
        double core = AndromedaModel::CoreDensity(x, y, z);

        glm::vec3 white(0.94f, 0.95f, 1.00f);
        glm::vec3 blue(0.78f, 0.86f, 1.00f);
        glm::vec3 pink(1.00f, 0.80f, 0.88f);
        glm::vec3 warm(1.00f, 0.80f, 0.62f);
        glm::vec3 color = glm::mix(white, blue, 0.18f);
        color = glm::mix(color, glm::mix(blue, pink, static_cast<float>(0.5 + 0.5 * std::sin(radius / 1500.0 + angle * 1.35))), static_cast<float>(0.26 * std::pow(arm, 0.55)));
        color = glm::mix(color, warm, static_cast<float>(0.64 * AndromedaModel::Smooth01(bulge)));
        color = glm::mix(color, glm::vec3(1.00f, 0.95f, 0.78f), static_cast<float>(0.72 * AndromedaModel::Smooth01(core)));
        return color;
    }
}

AndromedaDensityRenderer::AndromedaDensityRenderer(std::size_t particleCount)
{
    Build(particleCount);
    CreateBuffers();
}

AndromedaDensityRenderer::~AndromedaDensityRenderer()
{
    if (vbo != 0)
        glDeleteBuffers(1, &vbo);

    if (vao != 0)
        glDeleteVertexArrays(1, &vao);
}

void AndromedaDensityRenderer::Build(std::size_t particleCount)
{
    particles.clear();
    particles.reserve(particleCount);

    std::size_t baseCount = static_cast<std::size_t>(particleCount * 0.14);
    std::size_t cloudCount = static_cast<std::size_t>(particleCount * 0.72);
    std::size_t dustCount = particleCount - baseCount - cloudCount;

    std::mt19937_64 rng(0x414E44524F4D4544ull);
    double diskRadius = AndromedaModel::DiskRadiusParsec;

    for (std::size_t i = 0; i < baseCount; i++)
    {
        double radius;
        double angle;
        double z;

        if (Uniform01(rng) < 0.36)
        {
            std::normal_distribution<double> xDist(0.0, 4200.0);
            std::normal_distribution<double> yDist(0.0, 2600.0);
            std::normal_distribution<double> zDist(0.0, 1800.0);
            double x = xDist(rng);
            double y = yDist(rng);
            z = zDist(rng);
            radius = std::sqrt(x * x + y * y);
            angle = std::atan2(y, x);
        }
        else
        {
            radius = SampleDiskRadius(rng, 7200.0, 500.0, diskRadius - 120.0);
            angle = Uniform01(rng) * AndromedaModel::TwoPi;
            double height = AndromedaModel::DiskScaleHeight(radius);
            z = SampleSignedExponential(rng, height * 0.68);
        }

        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        double arm = AndromedaModel::ArmDensity(radius, angle);
        double edge = AndromedaModel::DiskEdgeTaper(radius);
        double keepProbability = std::clamp((0.15 + 0.24 * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.95)) + 0.44 * std::pow(arm, 0.52) + 0.62 * AndromedaModel::BulgeDensity(x, y, z)) * edge, 0.0, 1.0);

        if (Uniform01(rng) > keepProbability)
        {
            i--;
            continue;
        }

        Particle particle;
        particle.Position = TransformToWorld(x, y, z);
        particle.Color = StellarColor(x, y, z);
        particle.Brightness = RandomBrightness(rng, 0.015f, 0.065f) * static_cast<float>(0.78 + 0.32 * std::pow(arm, 0.42) + 0.34 * AndromedaModel::CoreDensity(x, y, z));
        particle.Size = 0.92f + static_cast<float>(Uniform01(rng)) * 0.82f;
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

        if (Uniform01(rng) < 0.32)
        {
            std::normal_distribution<double> xDist(0.0, 5200.0);
            std::normal_distribution<double> yDist(0.0, 3400.0);
            std::normal_distribution<double> zDist(0.0, 1900.0);
            double x = xDist(rng);
            double y = yDist(rng);
            z = zDist(rng);
            radius = std::sqrt(x * x + y * y);
            angle = std::atan2(y, x);
        }
        else if (Uniform01(rng) < 0.28)
        {
            radius = SampleAreaRadius(rng, 8500.0, diskRadius - 100.0);
            angle = Uniform01(rng) * AndromedaModel::TwoPi;
            double height = AndromedaModel::DiskScaleHeight(radius);
            z = SampleSignedExponential(rng, height * 0.76);
        }
        else
        {
            radius = SampleDiskRadius(rng, 7600.0, 1200.0, diskRadius - 100.0);
            angle = Uniform01(rng) * AndromedaModel::TwoPi;
            double height = AndromedaModel::DiskScaleHeight(radius);
            z = SampleSignedExponential(rng, height * 0.84);
        }

        double x = radius * std::cos(angle);
        double y = radius * std::sin(angle);
        double density = AndromedaModel::CloudDensity(x, y, z);

        if (Uniform01(rng) > density)
        {
            i--;
            continue;
        }

        double arm = AndromedaModel::ArmDensity(radius, angle);
        double bulge = AndromedaModel::BulgeDensity(x, y, z);
        double core = AndromedaModel::CoreDensity(x, y, z);
        double cloudWeight = 0.56 + 0.30 * std::pow(arm, 0.58) + 0.18 * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.84)) + 0.50 * bulge + 0.42 * core;

        Particle particle;
        particle.Position = TransformToWorld(x, y, z);
        particle.Color = CloudColor(x, y, z);
        particle.Brightness = RandomBrightness(rng, 0.030f, 0.102f) * static_cast<float>(std::clamp(cloudWeight, 0.0, 1.45));
        particle.Size = 7.0f + static_cast<float>(Uniform01(rng)) * 15.0f;
        particle.Mode = 1.0f;
        particle.Seed = static_cast<float>(Uniform01(rng));
        particles.push_back(particle);
    }

    cloudParticleCount = particles.size() - baseParticleCount;

    for (std::size_t i = 0; i < dustCount; i++)
    {
        double radius = SampleDiskRadius(rng, 7000.0, 2600.0, std::min(AndromedaModel::DustRadiusParsec, diskRadius - 250.0));
        double angle = Uniform01(rng) * AndromedaModel::TwoPi;
        double arm = AndromedaModel::ArmDensity(radius, angle);
        double edge = AndromedaModel::DiskEdgeTaper(radius);

        double patchField = 0.54
            + 0.24 * std::sin(radius / 920.0 + angle * 2.0)
            + 0.16 * std::sin(radius / 380.0 - angle * 4.0)
            + 0.08 * std::sin(radius / 165.0 + angle * 6.5);
        patchField = std::clamp(patchField, 0.0, 1.0);

        double keepProbability = (0.018 + 0.32 * std::pow(arm, 0.74) + 0.08 * (1.0 - std::pow(std::clamp(arm, 0.0, 1.0), 0.90))) * (0.30 + 0.70 * patchField) * edge;

        if (Uniform01(rng) > keepProbability)
        {
            i--;
            continue;
        }

        double height = AndromedaModel::DiskScaleHeight(radius);
        double z = SampleSignedExponential(rng, height * (0.48 + 0.22 * Uniform01(rng)));

        double radialJitter = (Uniform01(rng) - 0.5) * (260.0 + 520.0 * Uniform01(rng));
        double tangentialJitter = (Uniform01(rng) - 0.5) * (340.0 + 760.0 * Uniform01(rng));

        double cosA = std::cos(angle);
        double sinA = std::sin(angle);
        double x = radius * cosA + radialJitter * cosA - tangentialJitter * sinA;
        double y = radius * sinA + radialJitter * sinA + tangentialJitter * cosA;

        float dustHue = static_cast<float>(0.5 + 0.5 * std::sin(radius / 1200.0 + angle * 1.8 - z / 220.0));
        glm::vec3 dustCool(0.018f, 0.020f, 0.032f);
        glm::vec3 dustMauve(0.028f, 0.022f, 0.036f);
        glm::vec3 dustWarm(0.032f, 0.027f, 0.021f);

        Particle particle;
        particle.Position = TransformToWorld(x, y, z);
        particle.Color = glm::mix(glm::mix(dustCool, dustMauve, dustHue), dustWarm, static_cast<float>(0.18 + 0.22 * patchField));
        particle.Brightness = RandomBrightness(rng, 0.007f, 0.022f) * static_cast<float>(0.40 + 0.32 * std::pow(arm, 0.72) + 0.28 * patchField);
        particle.Size = 4.5f + static_cast<float>(Uniform01(rng)) * 9.5f;
        particle.Mode = 2.0f;
        particle.Seed = static_cast<float>(Uniform01(rng));
        particles.push_back(particle);
    }

    dustParticleCount = particles.size() - baseParticleCount - cloudParticleCount;
}

void AndromedaDensityRenderer::CreateBuffers()
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

void AndromedaDensityRenderer::Render() const
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glBindVertexArray(vao);
    glDrawArrays(GL_POINTS, 0, static_cast<GLsizei>(baseParticleCount));
    glBindVertexArray(0);

    glDisable(GL_BLEND);
}

void AndromedaDensityRenderer::RenderClouds() const
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

void AndromedaDensityRenderer::RenderDust() const
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

std::size_t AndromedaDensityRenderer::GetParticleCount() const
{
    return particles.size();
}
