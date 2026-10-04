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
const float TWO_PI = 6.28318530717958647692;
const float CELL_SIZE = 72.0;
const float LOCAL_RANGE = 300.0;
const int GRID_X = 9;
const int GRID_Y = 9;
const int GRID_Z = 7;

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

float ArmContribution(float radius, float angle, float phase, float pitchDegrees, float minRadius, float maxRadius, float widthInner, float widthOuter, float strength, float wobblePhase)
{
    if (radius < minRadius || radius > maxRadius)
        return 0.0;

    float pitch = radians(pitchDegrees);
    float referenceRadius = max(minRadius, 2500.0);
    float center = phase + log(radius / referenceRadius) / tan(pitch);
    center += 0.075 * sin(radius / 1550.0 + wobblePhase);
    center += 0.032 * sin(radius / 590.0 + wobblePhase * 2.1);
    center += 0.014 * sin(radius / 245.0 + wobblePhase * 3.7);

    float t = clamp((radius - minRadius) / max(maxRadius - minRadius, 1.0), 0.0, 1.0);
    float width = mix(widthInner, widthOuter, t);
    float modulation = 0.88
        + 0.16 * sin(radius / 1180.0 + wobblePhase)
        + 0.08 * sin(radius / 410.0 + wobblePhase * 1.8);
    width *= clamp(modulation, 0.70, 1.18);

    float physicalDistance = abs(WrapAngle(angle - center)) * radius;
    float core = exp(-0.5 * physicalDistance * physicalDistance / max(width * width, 1.0));
    float shoulderWidth = width * 2.35;
    float shoulder = exp(-0.5 * physicalDistance * physicalDistance / max(shoulderWidth * shoulderWidth, 1.0));

    float segment = 0.66
        + 0.20 * sin(radius / 980.0 + wobblePhase)
        + 0.12 * sin(radius / 365.0 + wobblePhase * 2.4)
        + 0.06 * sin(radius / 155.0 + wobblePhase * 4.3);
    segment = clamp(segment, 0.22, 1.0);

    return (core * 0.66 + shoulder * 0.34) * strength * segment;
}

float ArmDensity(float radius, float angle)
{
    float a = 0.0;
    a = max(a, ArmContribution(radius, angle, 0.12, 18.5, 2900.0, 14700.0, 760.0, 2200.0, 1.00, 0.2));
    a = max(a, ArmContribution(radius, angle, 1.67, 17.2, 3300.0, 14200.0, 800.0, 2300.0, 0.94, 1.5));
    a = max(a, ArmContribution(radius, angle, 3.20, 19.4, 3700.0, 14800.0, 840.0, 2380.0, 0.90, 2.8));
    a = max(a, ArmContribution(radius, angle, 4.78, 16.9, 2750.0, 13100.0, 720.0, 2100.0, 0.84, 4.1));
    a = max(a, ArmContribution(radius, angle, 2.55, 25.0, 6900.0, 10600.0, 430.0, 900.0, 0.30, 5.2));
    return clamp(a, 0.0, 1.0);
}

float DiskScaleHeight(float radius)
{
    float central = 260.0 * exp(-radius / 2800.0);
    float flare = 210.0 * Smooth01((radius - 6500.0) / 8000.0);
    return 270.0 + central + flare;
}

float BulgeDensity(vec3 p)
{
    float c = cos(radians(27.0));
    float s = sin(radians(27.0));
    float xr = p.x * c + p.y * s;
    float yr = -p.x * s + p.y * c;
    float q = xr * xr / (2500.0 * 2500.0)
        + yr * yr / (1050.0 * 1050.0)
        + p.z * p.z / (820.0 * 820.0);
    return exp(-0.5 * q);
}

float CloudDensity(vec3 p)
{
    float radius = length(p.xy);

    if (radius >= 15000.0)
        return 0.0;

    float angle = atan(p.y, p.x);
    float arm = ArmDensity(radius, angle);
    float edge = radius <= 12800.0 ? 1.0 : Smooth01((15000.0 - radius) / 2200.0);
    float vertical = exp(-abs(p.z) / max(DiskScaleHeight(radius), 1.0));
    float outer = Smooth01((radius - 5500.0) / 8500.0);
    float baseline = mix(0.15, 0.035, outer);
    float diskField = baseline + (0.72 + 0.12 * outer) * pow(arm, 0.34);
    float bulge = 0.68 * BulgeDensity(p);
    return clamp((diskField * vertical + bulge) * edge, 0.0, 1.0);
}

float LocalStarVerticalWeight(float radius, float z)
{
    float bulgeBlend = exp(-radius / 2600.0);
    float diskHeight = mix(170.0, 310.0, Smooth01((radius - 3200.0) / 9200.0));
    float coreHeight = 640.0;
    float scaleHeight = mix(diskHeight, coreHeight, bulgeBlend);
    float absZ = abs(z);
    float core = exp(-pow(absZ / max(scaleHeight, 1.0), 1.45));
    float shoulder = exp(-pow(absZ / max(scaleHeight * 1.90, 1.0), 2.35));
    return clamp(core * 0.82 + shoulder * 0.18, 0.0, 1.0);
}

float LocalStarProbability(vec3 p)
{
    float radius = length(p.xy);
    float angle = atan(p.y, p.x);
    float cloud = CloudDensity(p);
    float vertical = LocalStarVerticalWeight(radius, p.z);

    if (cloud < 0.035 || vertical < 0.010)
        return 0.0;

    float radial = exp((8200.0 - radius) / 7200.0);
    radial = clamp(radial, 0.42, 2.30);

    float cloudGate = Smooth01((cloud - 0.035) / 0.50);
    float arm = ArmDensity(radius, angle);
    float armBoost = mix(0.78, 1.18, pow(arm, 0.55));
    float centerBoost = 1.0 + 0.24 * BulgeDensity(p);
    float densityCompensation = 1.34;

    return clamp((0.11 + 0.83 * cloudGate) * radial * vertical * armBoost * centerBoost * densityCompensation, 0.0, 0.965);
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
    if (u < 0.68)
        return mix(vec3(1.0, 0.48, 0.25), vec3(1.0, 0.82, 0.62), u / 0.68);

    if (u < 0.92)
        return mix(vec3(1.0, 0.92, 0.80), vec3(0.80, 0.88, 1.0), (u - 0.68) / 0.24);

    return mix(vec3(0.72, 0.82, 1.0), vec3(0.60, 0.73, 1.0), (u - 0.92) / 0.08);
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
    vec3 cameraPosition = cameraOffsetHigh + cameraOffsetLow;

    if (CloudDensity(cameraPosition) < 0.10)
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
    vec3 relativePosition = worldPosition - cameraPosition;

    float distancePc = length(relativePosition);

    if (distancePc <= 0.001 || distancePc >= LOCAL_RANGE)
    {
        Cull();
        return;
    }

    float edgeFade = 1.0 - smoothstep(LOCAL_RANGE * 0.48, LOCAL_RANGE, distancePc);
    float spawnFade = 1.0 - smoothstep(LOCAL_RANGE * 0.58, LOCAL_RANGE, distancePc);
    float probability = LocalStarProbability(worldPosition);
    float accept = Random01(state);

    if (accept > probability)
    {
        Cull();
        return;
    }

    vec3 sourceDirection = relativePosition / distancePc;
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

    if (luminosityClass < 0.72)
        absoluteMagnitude = mix(5.2, 11.5, Random01(state));
    else if (luminosityClass < 0.94)
        absoluteMagnitude = mix(0.5, 5.0, Random01(state));
    else if (luminosityClass < 0.992)
        absoluteMagnitude = mix(-3.0, 1.5, Random01(state));
    else
        absoluteMagnitude = mix(-7.0, -2.5, Random01(state));

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
        vec3 previousRelative = relativePosition + normalize(observerVelocityDirection) * relativisticParams.z;
        float previousDistance = max(length(previousRelative), 0.000001);
        float ignoredDoppler = 1.0;
        vec3 previousDirection = AberrateDirection(previousRelative / previousDistance, normalize(observerVelocityDirection), gamma, ignoredDoppler);
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
    starColor = mix(vec3(luminance), starColor, 0.92) * mix(0.91, 1.60, shaped);
    starAlpha = visibility * pow(spawnFade, 1.15) * clamp(0.35 + 0.82 * shaped, 0.0, 1.0);

    if (relativistic)
    {
        starColor = ApplyDopplerColor(starColor, dopplerFactor);
        float logD = clamp(log2(max(dopplerFactor, 1.0e-12)), -24.0, 24.0);
        float displayBeaming = exp2(clamp(4.0 * logD / 48.0, -1.0, 1.0));
        starColor *= sqrt(displayBeaming);
        starAlpha *= clamp(displayBeaming, 0.42, 1.0);
    }
}
