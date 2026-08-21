#version 430 core
layout (location = 0) out vec3 gPos;
layout (location = 1) out vec3 gNorm;
layout (location = 2) out vec4 gAlbedoSpec;

layout (location = 0) in vec3 FragPos;
layout (location = 1) in vec3 Norm;
layout (location = 2) in vec2 TexCoords;

// uniform sampler2D texture_diffuse1;
// uniform sampler2D texture_specular1;
uniform sampler2D uTex0;
uniform int uUseTexture;

void main()
{
    gPos = FragPos;
    gNorm = normalize(Norm) * 0.5 + 0.5;
    vec3 albedo = vec3(1.0);

    if (uUseTexture == 1)
        albedo = texture(uTex0, TexCoord).rgb;

    gAlbedoSpec.rgb = albedo;
    gAlbedoSpec.a = 1.0;

    //gAlbedoSpec = vec4(0.9, 0.9, 0.9, 1.0);
    //gAlbedoSpec = vec4(1.0, 0.0, 0.0, 1.0);
}