#version 460 core

in vec3 starColor;

out vec4 FragColor;

void main()
{
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(p, p);

    if (r2 > 1.0)
        discard;

    float core = exp(-8.0 * r2);
    float glow = exp(-2.4 * r2);
    FragColor = vec4(starColor * (0.65 + 1.8 * core), 0.35 + 0.65 * glow);
}
