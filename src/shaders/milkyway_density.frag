#version 460 core

in vec3 particleColor;
in float particleAlpha;
in float particleMode;
in float particleSeed;

out vec4 FragColor;

float hash(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453123);
}

float valueNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);

    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p)
{
    float value = 0.0;
    float amplitude = 0.55;

    for (int i = 0; i < 5; i++)
    {
        value += amplitude * valueNoise(p);
        p = p * 2.03 + vec2(9.7, 13.1);
        amplitude *= 0.48;
    }

    return value;
}

void main()
{
    vec2 uv = gl_PointCoord * 2.0 - 1.0;
    float radius = length(uv);

    if (particleMode < 0.5)
    {
        if (radius > 1.0 || particleAlpha <= 0.0001)
            discard;

        float glow = exp(-2.25 * radius * radius);
        float alpha = particleAlpha * glow;
        FragColor = vec4(particleColor * alpha, alpha);
        return;
    }

    float rotation = particleSeed * 6.28318530718;
    float c = cos(rotation);
    float s = sin(rotation);
    mat2 rot = mat2(c, -s, s, c);
    vec2 p = rot * uv;

    float stretchX = 0.68 + 0.48 * fract(particleSeed * 17.17);
    float stretchY = 0.72 + 0.44 * fract(particleSeed * 29.73);
    p.x /= stretchX;
    p.y /= stretchY;

    float macroNoise = fbm(p * 2.7 + particleSeed * 5.3);
    float detailNoise = fbm(p * 7.8 - particleSeed * 3.1);
    float angle = atan(p.y, p.x);
    float boundary = 0.77
        + 0.145 * sin(angle * 2.0 + particleSeed * 15.0)
        + 0.105 * sin(angle * 3.0 - particleSeed * 9.0)
        + 0.085 * sin(angle * 5.0 + particleSeed * 21.0)
        + 0.16 * (macroNoise - 0.5);

    float warpedRadius = length(p);
    float edge = smoothstep(boundary + 0.10, boundary - 0.29, warpedRadius);

    if (edge <= 0.001 || particleAlpha <= 0.0001)
        discard;

    float textureNoise = 0.47 + 0.33 * macroNoise + 0.20 * detailNoise;
    float center = pow(clamp(1.0 - warpedRadius / max(boundary + 0.10, 0.26), 0.0, 1.0), 0.56);
    float veil = smoothstep(1.10, 0.18, warpedRadius) * (0.66 + 0.22 * macroNoise);
    float alpha = particleAlpha * max(edge * textureNoise * (0.56 + 0.44 * center), 0.30 * veil);

    if (particleMode < 1.5)
    {
        vec3 color = particleColor * (0.64 + 0.46 * textureNoise);
        FragColor = vec4(color * alpha, alpha);
    }
    else
    {
        float breakup = smoothstep(0.38, 0.72, macroNoise * 0.72 + detailNoise * 0.28);
        float dustAlpha = alpha * breakup * 0.38;
        vec3 dustColor = particleColor * (0.72 + 0.28 * textureNoise);
        FragColor = vec4(dustColor * dustAlpha, dustAlpha);
    }
}
