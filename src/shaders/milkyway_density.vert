#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aBrightness;
layout(location = 3) in float aSize;
layout(location = 4) in float aMode;
layout(location = 5) in float aSeed;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 relativisticParams;
uniform vec3 observerVelocityDirectionView;

out vec3 particleColor;
out float particleAlpha;
out float particleMode;
out float particleSeed;

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
    float shift = tanh(logD / 10.5);
    vec3 blueWhite = vec3(0.74, 0.87, 1.0);
    vec3 redWarm = vec3(1.0, 0.42, 0.20);

    if (shift >= 0.0)
        return mix(color, blueWhite, shift * 0.28);

    return mix(color, redWarm, -shift * 0.32);
}

void main()
{
    vec4 viewPosition = view * vec4(aPosition, 1.0);
    float distancePc = max(length(viewPosition.xyz), 1.0);
    float dopplerFactor = 1.0;
    bool relativistic = relativisticParams.x > 0.5;

    if (relativistic)
    {
        vec3 apparentDirection = AberrateDirection(
            viewPosition.xyz / distancePc,
            normalize(observerVelocityDirectionView),
            max(relativisticParams.y, 1.0),
            dopplerFactor
        );
        viewPosition.xyz = apparentDirection * distancePc;
    }

    float farResponse = clamp(9000.0 / distancePc, 0.64, 1.0);
    float brightness = aBrightness * mix(0.94, 1.16, farResponse);

    if (relativistic && aMode < 1.5)
    {
        float logD = clamp(log2(max(dopplerFactor, 1.0e-12)), -24.0, 24.0);
        float physicalLog2Intensity = 4.0 * logD;
        float displayBeaming = exp2(clamp(physicalLog2Intensity / 64.0, -0.70, 0.70));
        brightness *= displayBeaming;
    }

    gl_Position = projection * viewPosition;

    if (aMode < 0.5)
    {
        float localFade = smoothstep(8.0, 45.0, distancePc);
        gl_PointSize = clamp(aSize * (1.25 + brightness * 2.0), 1.1, 5.3);
        particleAlpha = localFade * clamp(0.035 + brightness * 0.25, 0.0, 0.16);
    }
    else if (aMode < 1.5)
    {
        float localFade = smoothstep(120.0, 520.0, distancePc);
        float cloudScale = clamp(7900.0 / distancePc, 0.55, 1.48);
        gl_PointSize = clamp(aSize * cloudScale, 2.2, 24.0);
        particleAlpha = localFade * clamp(0.017 + brightness * 0.45, 0.0, 0.118);
    }
    else
    {
        float localFade = smoothstep(90.0, 360.0, distancePc);
        float cloudScale = clamp(7200.0 / distancePc, 0.74, 1.40);
        gl_PointSize = clamp(aSize * cloudScale, 3.8, 18.5);
        particleAlpha = localFade * clamp(0.008 + brightness * 0.36, 0.0, 0.055);
    }

    particleColor = aColor;

    if (relativistic && aMode < 1.5)
        particleColor = ApplyDopplerColor(particleColor, dopplerFactor);

    particleMode = aMode;
    particleSeed = aSeed;
}
