#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;
layout(location = 2) in float aBrightness;
layout(location = 3) in float aSize;

uniform mat4 view;
uniform mat4 projection;

out vec3 starColor;
out float starBrightness;

void main()
{
    gl_Position = projection * view * vec4(aPosition, 1.0);
    gl_PointSize = aSize;

    starColor = aColor;
    starBrightness = aBrightness;
}