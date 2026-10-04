#version 460 core

layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aColor;

uniform mat4 view;
uniform mat4 projection;

out vec3 starColor;

void main()
{
    gl_Position = projection * view * vec4(aPosition, 1.0);
    gl_PointSize = 12.0;
    starColor = aColor;
}
