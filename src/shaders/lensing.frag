#version 460 core

in vec2 uv;

out vec4 FragColor;

uniform sampler2D sceneTexture;
uniform vec2 viewportSize;
uniform int lensCount;
uniform vec4 lenses[8];

void main()
{
    vec2 sampleUv = uv;
    float totalShadow = 0.0;

    for (int i = 0; i < lensCount; i++)
    {
        vec2 center = lenses[i].xy;
        float einsteinRadius = max(lenses[i].z, 0.001);
        float shadowRadius = max(lenses[i].w, 0.0);

        vec2 imagePixels = (uv - center) * viewportSize;
        float imageRadius = length(imagePixels);
        float effectRadius = einsteinRadius * 3.3;

        if (imageRadius < effectRadius)
        {
            float safeImageRadius = max(imageRadius, 0.001);
            vec2 imageDirection = imagePixels / safeImageRadius;

            float innerMappingRadius = max(shadowRadius * 1.35, einsteinRadius * 0.50);
            float lensRadius = max(imageRadius, innerMappingRadius);
            float rawSourceRadius = lensRadius - (einsteinRadius * einsteinRadius) / lensRadius;

            float sourceLimit = einsteinRadius * 1.8;
            float sourceRadius = sourceLimit * tanh(rawSourceRadius / max(sourceLimit, 0.001));

            vec2 sourcePixels = imageDirection * sourceRadius;
            vec2 warpedUv = center + sourcePixels / viewportSize;

            float outerFade = 1.0 - smoothstep(einsteinRadius * 1.65, effectRadius, imageRadius);
            float horizonFade = shadowRadius > 0.0
                ? smoothstep(shadowRadius * 0.98, shadowRadius * 1.16, imageRadius)
                : 1.0;

            float influence = outerFade * horizonFade;
            sampleUv = mix(sampleUv, warpedUv, influence);
        }

        if (shadowRadius > 0.0)
        {
            float shadow = 1.0 - smoothstep(shadowRadius * 0.90, shadowRadius * 1.04, imageRadius);
            totalShadow = max(totalShadow, shadow);
        }
    }

    vec3 color = texture(sceneTexture, clamp(sampleUv, vec2(0.0), vec2(1.0))).rgb;
    color *= 1.0 - totalShadow;

    FragColor = vec4(color, 1.0);
}
