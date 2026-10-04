#version 460 core

in vec3 starColor;
in float starAlpha;
in vec2 streakDirection;
in float streakAmount;
in float pointStretch;

out vec4 FragColor;

void main()
{
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    vec2 dir = normalize(abs(streakDirection.x) + abs(streakDirection.y) > 0.0001 ? streakDirection : vec2(1.0, 0.0));
    vec2 perp = vec2(-dir.y, dir.x);

    float along = dot(p, dir);
    float across = dot(p, perp);
    float stretch = max(pointStretch, 1.0);
    float coreR2 = across * across + (along * along) / (stretch * stretch);

    float glow = exp(-7.4 * coreR2);
    float core = exp(-30.0 * coreR2);
    float roundAlpha = clamp(glow * 0.24 + core * 1.0, 0.0, 1.0);
    float roundLight = 0.08 + glow * 0.34 + core * 1.15;

    float trail = 0.0;

    if (streakAmount > 0.05)
    {
        float longitudinal = 1.0 - smoothstep(-0.95, 0.92, along);
        float transverse = exp(-18.0 * across * across);
        trail = longitudinal * transverse * clamp(streakAmount / 18.0, 0.0, 1.0) * 0.72;
    }

    float alpha = starAlpha * clamp(max(roundAlpha, trail * 0.34), 0.0, 1.0);

    if (alpha < 0.006)
        discard;

    vec3 color = starColor * (roundLight + trail * 0.48);
    FragColor = vec4(color, alpha);
}
