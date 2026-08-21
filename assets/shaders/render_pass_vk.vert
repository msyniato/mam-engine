#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNorm;
layout(location = 2) in vec2 aTexCoords;

layout(location = 0) out vec3 FragPos;
layout(location = 1) out vec3 Norm;
layout(location = 2) out vec2 TexCoords;

void main() {
    FragPos = aPos;
    Norm = aNorm;
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 1.0);
}