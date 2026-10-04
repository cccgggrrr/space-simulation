#version 460 core

layout(location = 0) in vec3 aPositionHigh;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aMagnitude;
layout(location = 3) in vec3 aPositionLow;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraOffsetHigh;
uniform vec3 cameraOffsetLow;
uniform int magnitudeMode;
uniform vec3 relativisticParams;
uniform vec3 observerVelocityDirection;
uniform vec3 viewportInfo;

out vec3 starColor;
out float starAlpha;
out vec2 streakDirection;
out float streakAmount;
out float pointStretch;

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

void main()
{
    vec3 decodedLow = magnitudeMode == 1 ? aPositionLow * 0.002 : aPositionLow;
    vec3 relativePosition = (aPositionHigh - cameraOffsetHigh) + (decodedLow - cameraOffsetLow);
    float distancePc = max(length(relativePosition), 0.00000001);
    vec3 sourceDirection = relativePosition / distancePc;

    bool relativistic = relativisticParams.x > 0.5;
    float gamma = max(relativisticParams.y, 1.0);
    float dopplerFactor = 1.0;
    vec3 apparentDirection = sourceDirection;

    if (relativistic)
        apparentDirection = AberrateDirection(sourceDirection, normalize(observerVelocityDirection), gamma, dopplerFactor);

    vec4 viewPosition = view * vec4(apparentDirection * distancePc, 1.0);

    float sourceMagnitude = magnitudeMode == 1 ? aMagnitude * 0.01 : aMagnitude;
    float apparentMagnitude = sourceMagnitude;

    if (magnitudeMode == 1)
        apparentMagnitude = sourceMagnitude + 5.0 * (log(distancePc) / log(10.0)) - 5.0;

    float displayMagnitude = apparentMagnitude + (magnitudeMode == 0 ? 0.65 : -0.18);
    float visibility = magnitudeMode == 1
        ? 1.0 - smoothstep(11.0, 16.8, displayMagnitude)
        : 1.0 - smoothstep(10.0, 15.0, displayMagnitude);

    if (visibility <= 0.0001)
    {
        gl_Position = vec4(2.0, 2.0, 2.0, 1.0);
        gl_PointSize = 1.0;
        starColor = vec3(0.0);
        starAlpha = 0.0;
        streakDirection = vec2(1.0, 0.0);
        streakAmount = 0.0;
        pointStretch = 1.0;
        return;
    }

    float flux = pow(10.0, -0.4 * (displayMagnitude - 1.5));
    float brightness = clamp(flux, 0.0, 1.0);
    float shapedBrightness = pow(max(brightness, 0.000001), 0.30);
    float proximity = 1.0 + 8.0 * pow(clamp(0.50 / distancePc, 0.0, 1.0), 0.35);

    float nearColor = 1.0 - smoothstep(250.0, 5000.0, distancePc);
    float colorAmount = clamp(0.80 + nearColor * 0.12 + shapedBrightness * 0.06, 0.80, 0.98);
    float luminance = dot(aColor, vec3(0.2126, 0.7152, 0.0722));
    vec3 neutral = vec3(luminance);

    gl_Position = projection * viewPosition;

    streakDirection = vec2(1.0, 0.0);
    streakAmount = 0.0;

    if (relativistic && relativisticParams.z > 0.0 && gl_Position.w > 0.0)
    {
        vec3 previousRelativePosition = relativePosition + normalize(observerVelocityDirection) * relativisticParams.z;
        float previousDistance = max(length(previousRelativePosition), 0.00000001);
        float ignoredDoppler = 1.0;
        vec3 previousApparentDirection = AberrateDirection(
            previousRelativePosition / previousDistance,
            normalize(observerVelocityDirection),
            gamma,
            ignoredDoppler
        );

        vec4 previousViewPosition = view * vec4(previousApparentDirection * previousDistance, 1.0);
        vec4 previousClip = projection * previousViewPosition;

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

    float basePointSize = magnitudeMode == 1 ? 1.45 : 1.75;
    float pointGain = magnitudeMode == 1 ? 6.2 : 7.0;
    float unstretchedPointSize = clamp((basePointSize + pointGain * shapedBrightness * visibility) * proximity, basePointSize, 22.0);
    float finalPointSize = clamp(unstretchedPointSize + streakAmount * 0.88, basePointSize, 320.0);
    gl_PointSize = finalPointSize;
    pointStretch = finalPointSize / max(unstretchedPointSize, 0.001);

    float proceduralGain = magnitudeMode == 1 ? 1.08 : 1.0;
    starColor = mix(neutral, aColor, colorAmount) * mix(0.72, 1.42, shapedBrightness) * proceduralGain;
    starAlpha = visibility * clamp(0.25 + 0.84 * shapedBrightness, 0.0, 1.0);

    if (relativistic)
    {
        starColor = ApplyDopplerColor(starColor, dopplerFactor);
        float logD = clamp(log2(max(dopplerFactor, 1.0e-12)), -24.0, 24.0);
        float physicalLog2Intensity = 4.0 * logD;
        float displayBeaming = exp2(clamp(physicalLog2Intensity / 48.0, -1.0, 1.0));
        starColor *= sqrt(displayBeaming);
        starAlpha *= clamp(displayBeaming, 0.42, 1.0);
    }
}
