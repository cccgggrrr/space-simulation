#version 460 core

in vec3 starColor;
in float starAlpha;
in vec2 streakDirection;
in float streakAmount;
in float pointStretch;
in float starRenderMode;

out vec4 FragColor;

void main()
{
    if (starAlpha <= 0.0001)
        discard;

    vec2 p = gl_PointCoord * 2.0 - 1.0;
    vec2 coreP = p * pointStretch;
    float coreR2 = dot(coreP, coreP);

    bool localStar = starRenderMode > 1.5;
    float glow = exp(-(localStar ? 7.2 : 3.0) * coreR2);
    float core = exp(-(localStar ? 28.0 : 17.0) * coreR2);
    float roundAlpha = localStar
        ? clamp(glow * 0.24 + core * 1.0, 0.0, 1.0)
        : clamp(glow * 0.56 + core * 0.88, 0.0, 1.0);
    float roundLight = localStar
        ? 0.10 + glow * 0.34 + core * 1.16
        : 0.18 + glow * 0.92 + core * 0.70;

    float trailFactor = clamp(streakAmount / 18.0, 0.0, 1.0);
    float trail = 0.0;

    if (trailFactor > 0.001)
    {
        vec2 direction = normalize(streakDirection);
        vec2 perpendicular = vec2(-direction.y, direction.x);
        float along = dot(p, direction);
        float across = dot(p, perpendicular);
        float extent = mix(0.24, 0.92, trailFactor);
        float width = mix(0.095, 0.045, trailFactor);
        float startMask = smoothstep(-0.04, 0.035, along);
        float endMask = 1.0 - smoothstep(extent * 0.72, extent, along);
        float transverse = exp(-(across * across) / max(width * width, 0.00001));
        float longitudinal = exp(-2.15 * max(along, 0.0) / max(extent, 0.05));
        trail = startMask * endMask * transverse * longitudinal * trailFactor * (localStar ? 0.82 : 1.0);
    }

    float alpha = starAlpha * clamp(max(roundAlpha, trail * (localStar ? 0.40 : 0.48)), 0.0, 1.0);
    vec3 color = starColor * (roundLight + trail * (localStar ? 0.56 : 0.82));

    if (alpha <= 0.0001)
        discard;

    FragColor = vec4(color, alpha);
}
