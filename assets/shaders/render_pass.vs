#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNorm;
layout (location = 2) in vec2 aTexCoords;

out vec3 FragPos;
out vec3 Norm;
out vec2 TexCoords;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;

    Norm = mat3(transpose(inverse(model))) * aNorm;
    
    TexCoords = aTexCoords;

    gl_Position = projection * view * worldPos;
}