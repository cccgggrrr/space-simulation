#version 460 core

in vec3 starColor;
in float starBrightness;

out vec4 FragColor;

void main()
{
    vec2 point = gl_PointCoord * 2.0 - 1.0;
    float distanceFromCenter = dot(point, point);

    if (distanceFromCenter > 1.0)
        discard;

    float glow = 1.0 - distanceFromCenter;
    FragColor = vec4(starColor * starBrightness * (0.5 + glow), 1.0);
}