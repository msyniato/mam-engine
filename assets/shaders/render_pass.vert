#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoords;

layout (location = 0) out vec3 FragPos;
layout (location = 1) out vec3 Norm;
layout (location = 2) out vec2 TexCoords;

layout(set = 0, binding = 0) uniform UBO
{
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main()
{
    vec4 worldPos = ubo.model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    Norm = mat3(transpose(inverse(ubo.model))) * aNorm;
    TexCoords = aTexCoords;

    gl_Position = ubo.projection * ubo.view * worldPos;
}