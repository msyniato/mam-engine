#version 330 core
out vec4 FragColor;

uniform vec4 color;

in vec3 norm;
in vec2 uv;

void main()
{
    FragColor = vec4(normalize(norm) * 0.5 + 0.5, 1.0);
} 