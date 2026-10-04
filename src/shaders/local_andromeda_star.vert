#version 460 core

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraOffsetHigh;
uniform vec3 cameraOffsetLow;
uniform vec3 relativisticParams;
uniform vec3 observerVelocityDirection;
uniform vec3 viewportInfo;

out vec3 starColor;
out float starAlpha;
out vec2 streakDirection;
out float streakAmount;
out float pointStretch;

const float PI = 3.14159265358979323846;
const float CELL_SIZE = 72.0;
const float LOCAL_RANGE = 300.0;
const int GRID_X = 9;
const int GRID_Y = 9;
const int GRID_Z = 7;

const vec3 ANDROMEDA_CENTER = vec3(-382641.36, 618902.51, -285986.99);
const vec3 ANDROMEDA_MAJOR = vec3(-0.69372805, -0.08946034, 0.71465953);
const vec3 ANDROMEDA_MINOR = vec3(-0.34853015, 0.91003810, -0.22440452);
const vec3 ANDROMEDA_NORMAL = vec3(-0.63029209, -0.40475610, -0.66249858);

uint Hash(uint x)
{
    x ^= x >> 16;
    x *= 0x7feb352du;
    x ^= x >> 15;
    x *= 0x846ca68bu;
    x ^= x >> 16;
    return x;
}

uint Hash4(ivec3 cell, uint index)
{
    uint h = Hash(uint(cell.x) + 0x9e3779b9u);
    h ^= Hash(uint(cell.y) + 0x85ebca6bu);
    h ^= Hash(uint(cell.z) + 0xc2b2ae35u);
    h ^= Hash(index + 0x27d4eb2fu);
    return Hash(h);
}

float Random01(inout uint state)
{
    state = Hash(state + 0x9e3779b9u);
    return float(state) * (1.0 / 4294967295.0);
}

float Smooth01(float x)
{
    x = clamp(x, 0.0, 1.0);
    return x * x * (3.0 - 2.0 * x);
}

float WrapAngle(float a)
{
    return atan(sin(a), cos(a));
}

vec3 GalacticToAndromeda(vec3 galactic)
{
    vec3 d = galactic - ANDROMEDA_CENTER;
    return vec3(dot(d, ANDROMEDA_MAJOR), dot(d, ANDROMEDA_MINOR), dot(d, ANDROMEDA_NORMAL));
}

vec3 AndromedaVectorToGalactic(vec3 localVector)
{
    return ANDROMEDA_MAJOR * localVector.x + ANDROMEDA_MINOR * localVector.y + ANDROMEDA_NORMAL * localVector.z;
}

float ArmContribution(float radius, float angle, float phase, float pitchDegrees, float minRadius, float maxRadius, float widthInner, float widthOuter, float strength, float wobblePhase)
{
    if (radius < minRadius || radius > maxRadius)
        return 0.0;

    float pitch = radians(pitchDegrees);
    float referenceRadius = max(minRadius, 3500.0);
    float center = phase + log(radius / referenceRadius) / tan(pitch);
    center += 0.060 * sin(radius / 2300.0 + wobblePhase);
    center += 0.026 * sin(radius / 720.0 + wobblePhase * 2.1);
    center += 0.012 * sin(radius / 260.0 + wobblePhase * 4.2);

    float t = clamp((radius - minRadius) / max(maxRadius - minRadius, 1.0), 0.0, 1.0);
    float width = mix(widthInner, widthOuter, t);
    float modulation = 0.92
        + 0.10 * sin(radius / 1500.0 + wobblePhase)
        + 0.05 * sin(radius / 460.0 + wobblePhase * 1.9);
    width *= clamp(modulation, 0.80, 1.16);

    float physicalDistance = abs(WrapAngle(angle - center)) * radius;
    float core = exp(-0.5 * physicalDistance * physicalDistance / max(width * width, 1.0));
    float shoulderWidth = width * 2.60;
    float shoulder = exp(-0.5 * physicalDistance * physicalDistance / max(shoulderWidth * shoulderWidth, 1.0));

    float segment = 0.72
        + 0.14 * sin(radius / 1150.0 + wobblePhase)
        + 0.10 * sin(radius / 410.0 + wobblePhase * 2.3)
        + 0.04 * sin(radius / 185.0 + wobblePhase * 3.7);
    segment = clamp(segment, 0.30, 1.0);

    return (core * 0.52 + shoulder * 0.48) * strength * segment;
}

float ArmDensity(float radius, float angle)
{
    float a = 0.0;
    a = max(a, ArmContribution(radius, angle, 0.10, 9.5, 4200.0, 31000.0, 1800.0, 5200.0, 1.00, 0.2));
    a = max(a, ArmContribution(radius, angle, PI + 0.30, 8.9, 4200.0, 32000.0, 1700.0, 5400.0, 0.94, 1.3));
    a = max(a, ArmContribution(radius, angle, 1.45, 12.0, 6500.0, 25500.0, 2800.0, 6200.0, 0.36, 2.4));
    a = max(a, ArmContribution(radius, angle, 4.65, 12.8, 6800.0, 25000.0, 3000.0, 6400.0, 0.30, 3.7));
    return clamp(a, 0.0, 1.0);
}

float DiskEdgeTaper(float radius)
{
    if (radius <= 26500.0)
        return 1.0;
    if (radius >= 33000.0)
        return 0.0;
    return Smooth01((33000.0 - radius) / 6500.0);
}

float DiskScaleHeight(float radius)
{
    float coreLift = 650.0 * exp(-radius / 4500.0);
    float outerFlare = 380.0 * Smooth01((radius - 12000.0) / 14000.0);
    return 340.0 + coreLift + outerFlare;
}

float BulgeDensity(vec3 p)
{
    float q = p.x * p.x / (6400.0 * 6400.0)
        + p.y * p.y / (4200.0 * 4200.0)
        + p.z * p.z / (3000.0 * 3000.0);
    return exp(-0.5 * q);
}

float CoreDensity(vec3 p)
{
    float q = p.x * p.x / (2100.0 * 2100.0)
        + p.y * p.y / (1500.0 * 1500.0)
        + p.z * p.z / (1200.0 * 1200.0);
    return exp(-0.5 * q);
}

float CloudDensity(vec3 p)
{
    float radius = length(p.xy);
    if (radius >= 33000.0)
        return 0.0;

    float angle = atan(p.y, p.x);
    float arm = ArmDensity(radius, angle);
    float edge = DiskEdgeTaper(radius);
    float vertical = exp(-abs(p.z) / max(DiskScaleHeight(radius), 1.0));
    float bulge = BulgeDensity(p);
    float core = CoreDensity(p);
    float outer = Smooth01((radius - 9500.0) / 17000.0);
    float inner = 1.0 - Smooth01((radius - 4200.0) / 5200.0);

    float diskBase = mix(0.12, 0.045, outer);
    float broadArm = 0.55 * pow(clamp(arm, 0.0, 1.0), 0.58);
    float interArm = 0.12 * (1.0 - pow(clamp(arm, 0.0, 1.0), 0.95));
    float bulgeField = 0.72 * bulge + 0.55 * core;
    float field = (diskBase + broadArm + interArm) * vertical + bulgeField * (0.75 + 0.25 * inner);
    return clamp(field * edge, 0.0, 1.0);
}

float LocalStarScaleHeight(float radius)
{
    float centralBulge = 900.0 * exp(-radius / 2400.0);
    float disk = 145.0 + 110.0 * exp(-radius / 7200.0);
    float flare = 65.0 * Smooth01((radius - 14000.0) / 12000.0);
    return disk + centralBulge + flare;
}

float LocalStarVerticalWeight(vec3 p)
{
    float radius = length(p.xy);
    float height = LocalStarScaleHeight(radius);
    float absZ = abs(p.z);
    float soft = exp(-absZ / max(height, 1.0));
    float clampFade = 1.0 - smoothstep(height * 1.7, height * 2.8, absZ);
    return soft * clampFade;
}

float LocalStarProbability(vec3 p)
{
    float radius = length(p.xy);
    if (radius >= 33000.0)
        return 0.0;

    float cloud = CloudDensity(p);
    float vertical = LocalStarVerticalWeight(p);
    if (vertical < 0.008)
        return 0.0;

    float arm = ArmDensity(radius, atan(p.y, p.x));
    float edge = DiskEdgeTaper(radius);
    float bulge = BulgeDensity(p);
    float core = CoreDensity(p);
    float radial = exp((12000.0 - radius) / 14500.0);
    radial = clamp(radial, 0.56, 1.95);

    float cloudGate = Smooth01((cloud - 0.012) / 0.34);
    float armBoost = 0.94 + 0.34 * pow(arm, 0.48);
    float centerBoost = 1.0 + 0.55 * bulge + 0.45 * core;
    float visualCompression = pow(max(radial * centerBoost, 0.02), 0.24);
    float trillionScale = 1.78;

    return clamp((0.12 + 0.96 * cloudGate) * vertical * edge * armBoost * visualCompression * trillionScale, 0.0, 0.985);
}

vec3 AberrateDirection(vec3 direction, vec3 velocityDirection, float gamma, out float dopplerFactor)
{
    float safeGamma = max(gamma, 1.0);
    float gammaRoot = sqrt(max(safeGamma * safeGamma - 1.0, 0.0));
    float oneMinusBeta = 1.0 / max(safeGamma * (safeGamma + gammaRoot), 1.0);
    float mu = clamp(dot(direction, velocityDirection), -1.0, 1.0);
    float denominator = max((1.0 + mu) - mu * oneMinusBeta, 1.0e-20);
    float parallelComponent = (mu + 1.0 - oneMinusBeta) / denominator;
    vec3 perpendicular = direction - velocityDirection * mu;
    float perpendicularScale = 1.0 / (safeGamma * denominator);

    dopplerFactor = max(safeGamma * denominator, 1.0e-12);

    vec3 result = velocityDirection * parallelComponent + perpendicular * perpendicularScale;
    float resultLength = length(result);
    return resultLength > 1.0e-12 ? result / resultLength : direction;
}

vec3 ApplyDopplerColor(vec3 color, float dopplerFactor)
{
    float logD = clamp(log2(max(dopplerFactor, 1.0e-12)), -24.0, 24.0);
    float shift = tanh(logD / 9.0);
    vec3 blueWhite = vec3(0.72, 0.86, 1.0);
    vec3 redWarm = vec3(1.0, 0.38, 0.16);

    if (shift >= 0.0)
        return mix(color, blueWhite, shift * 0.46);

    return mix(color, redWarm, -shift * 0.52);
}

vec3 TemperatureColor(float u)
{
    if (u < 0.70)
        return mix(vec3(1.0, 0.52, 0.28), vec3(1.0, 0.84, 0.66), u / 0.70);

    if (u < 0.93)
        return mix(vec3(1.0, 0.93, 0.82), vec3(0.82, 0.89, 1.0), (u - 0.70) / 0.23);

    return mix(vec3(0.74, 0.84, 1.0), vec3(0.62, 0.75, 1.0), (u - 0.93) / 0.07);
}

void Cull()
{
    gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
    gl_PointSize = 1.0;
    starColor = vec3(0.0);
    starAlpha = 0.0;
    streakDirection = vec2(1.0, 0.0);
    streakAmount = 0.0;
    pointStretch = 1.0;
}

void main()
{
    vec3 cameraGalactic = cameraOffsetHigh + cameraOffsetLow;
    vec3 cameraPosition = GalacticToAndromeda(cameraGalactic);

    float cameraRadius = length(cameraPosition.xy);
    float cameraHeightLimit = max(LocalStarScaleHeight(cameraRadius) * 3.2, 2200.0);
    if (cameraRadius >= 33800.0 || abs(cameraPosition.z) > cameraHeightLimit)
    {
        Cull();
        return;
    }

    int instance = gl_InstanceID;
    int ix = instance % GRID_X;
    int iy = (instance / GRID_X) % GRID_Y;
    int iz = instance / (GRID_X * GRID_Y);

    ivec3 cameraChunk = ivec3(floor(cameraPosition / CELL_SIZE));
    ivec3 chunkOffset = ivec3(ix - GRID_X / 2, iy - GRID_Y / 2, iz - GRID_Z / 2);
    ivec3 chunk = cameraChunk + chunkOffset;

    uint state = Hash4(chunk, uint(gl_VertexID));
    vec3 randomOffset = vec3(Random01(state), Random01(state), Random01(state));
    vec3 worldPosition = (vec3(chunk) + randomOffset) * CELL_SIZE;
    vec3 relativeLocal = worldPosition - cameraPosition;

    float distancePc = length(relativeLocal);
    if (distancePc <= 0.001 || distancePc >= LOCAL_RANGE)
    {
        Cull();
        return;
    }

    float edgeFade = 1.0 - smoothstep(LOCAL_RANGE * 0.48, LOCAL_RANGE, distancePc);
    float spawnFade = 1.0 - smoothstep(LOCAL_RANGE * 0.58, LOCAL_RANGE, distancePc);
    float probability = LocalStarProbability(worldPosition);

    if (Random01(state) > probability)
    {
        Cull();
        return;
    }

    vec3 relativeGalactic = AndromedaVectorToGalactic(relativeLocal);
    vec3 sourceDirection = relativeGalactic / distancePc;
    bool relativistic = relativisticParams.x > 0.5;
    float gamma = max(relativisticParams.y, 1.0);
    float dopplerFactor = 1.0;
    vec3 apparentDirection = sourceDirection;

    if (relativistic)
        apparentDirection = AberrateDirection(sourceDirection, normalize(observerVelocityDirection), gamma, dopplerFactor);

    vec4 viewPosition = view * vec4(apparentDirection * distancePc, 1.0);
    gl_Position = projection * viewPosition;

    float spectral = Random01(state);
    starColor = TemperatureColor(spectral);

    float luminosityClass = Random01(state);
    float absoluteMagnitude;

    if (luminosityClass < 0.68)
        absoluteMagnitude = mix(5.0, 11.2, Random01(state));
    else if (luminosityClass < 0.93)
        absoluteMagnitude = mix(0.3, 5.0, Random01(state));
    else if (luminosityClass < 0.991)
        absoluteMagnitude = mix(-3.4, 1.4, Random01(state));
    else
        absoluteMagnitude = mix(-7.2, -2.8, Random01(state));

    float apparentMagnitude = absoluteMagnitude + 5.0 * (log(distancePc) / log(10.0)) - 5.0;
    float visibility = 1.0 - smoothstep(12.5, 17.0, apparentMagnitude);
    visibility *= edgeFade * pow(spawnFade, 1.25);

    if (visibility <= 0.00001)
    {
        Cull();
        return;
    }

    float flux = pow(10.0, -0.4 * (apparentMagnitude - 1.0));
    float brightness = clamp(flux, 0.0, 1.0);
    float shaped = pow(max(brightness, 0.000001), 0.28);
    float proximity = 1.0 + 2.0 * pow(clamp(0.45 / distancePc, 0.0, 1.0), 0.35);

    float baseSize = 20.0;
    float unstretched = clamp((baseSize + 5.8 * shaped) * proximity, baseSize, 28.5);

    streakDirection = vec2(1.0, 0.0);
    streakAmount = 0.0;

    if (relativistic && relativisticParams.z > 0.0 && gl_Position.w > 0.0)
    {
        vec3 previousRelativeGalactic = relativeGalactic + normalize(observerVelocityDirection) * relativisticParams.z;
        float previousDistance = max(length(previousRelativeGalactic), 0.000001);
        float ignoredDoppler = 1.0;
        vec3 previousDirection = AberrateDirection(previousRelativeGalactic / previousDistance, normalize(observerVelocityDirection), gamma, ignoredDoppler);
        vec4 previousView = view * vec4(previousDirection * previousDistance, 1.0);
        vec4 previousClip = projection * previousView;

        if (previousClip.w > 0.0)
        {
            vec2 currentNdc = gl_Position.xy / gl_Position.w;
            vec2 previousNdc = previousClip.xy / previousClip.w;
            vec2 trailNdc = previousNdc - currentNdc;
            vec2 trailPixels = vec2(trailNdc.x * viewportInfo.x * 0.5, -trailNdc.y * viewportInfo.y * 0.5);
            float pixels = length(trailPixels);

            if (pixels > 0.01)
                streakDirection = trailPixels / pixels;

            float visualPixels = pixels <= 18.0 ? pixels : 18.0 + 12.0 * log2(max(pixels / 18.0, 1.0));
            streakAmount = clamp(visualPixels, 0.0, 320.0);
        }
    }

    float finalSize = clamp(unstretched + streakAmount * 0.82, baseSize, 320.0);
    gl_PointSize = finalSize;
    pointStretch = finalSize / max(unstretched, 0.001);

    float luminance = dot(starColor, vec3(0.2126, 0.7152, 0.0722));
    starColor = mix(vec3(luminance), starColor, 0.42) * mix(0.92, 1.46, shaped);
    starAlpha = visibility * pow(spawnFade, 1.15) * clamp(0.40 + 0.86 * shaped, 0.0, 1.0);

    if (relativistic)
    {
        starColor = ApplyDopplerColor(starColor, dopplerFactor);
        float logD = clamp(log2(max(dopplerFactor, 1.0e-12)), -24.0, 24.0);
        float displayBeaming = exp2(clamp(4.0 * logD / 48.0, -1.0, 1.0));
        starColor *= sqrt(displayBeaming);
        starAlpha *= clamp(displayBeaming, 0.42, 1.0);
    }
}
