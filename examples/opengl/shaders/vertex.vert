#version 330 core

layout(location = 0) in vec2 v;
layout(location = 1) in vec2 uv;
layout(location = 2) in vec4 color;

uniform mat4 matrix;

out vec2 UV;
out vec4 Color;

void main()
{
    gl_Position = matrix * vec4(v, 0.0, 1.0);
    UV = uv;
    Color = color;
}
