#version 450 core
layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 FragPos;
layout(location = 1) in vec3 Norm;
layout(location = 2) in vec2 TexCoords;

void main() {
    outColor = vec4(0.9, 0.9, 0.9, 1.0);
}