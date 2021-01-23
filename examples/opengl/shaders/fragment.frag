#version 330 core

uniform sampler2D map;

in vec2 UV;
in vec4 Color;

out vec4 Out;

void main()
{
    float alpha = texture(map, UV).r;
    Out = vec4(Color.xyz, alpha);
}
