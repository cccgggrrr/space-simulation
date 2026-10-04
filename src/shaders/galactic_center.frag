#version 460 core

out vec4 FragColor;

void main()
{
    vec2 p = gl_PointCoord * 2.0 - 1.0;
    float r2 = dot(p, p);

    if (r2 > 1.0)
        discard;

    float core = exp(-7.0 * r2);
    float glow = exp(-2.0 * r2);
    vec3 color = vec3(1.0, 0.62, 0.22) * (0.7 + 1.8 * core);

    FragColor = vec4(color, 0.35 + 0.65 * glow);
}
