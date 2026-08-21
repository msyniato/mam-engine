#version 430 core
layout (location = 0) out vec3 gPos;
layout (location = 1) out vec3 gNorm;
layout (location = 2) out vec4 gAlbedoSpec;

in vec3 FragPos;
in vec3 Norm;
in vec2 TexCoords;

// uniform sampler2D texture_diffuse1;
// uniform sampler2D texture_specular1;
uniform sampler2D uTex0;
uniform int uUseTexture;

void main()
{
    gPos = FragPos;
    gNorm = normalize(Norm) * 0.5 + 0.5;
    //vec3 albedo = vec3(1.0);

        //albedo = texture(uTex0, TexCoords).rgb;
    //if (uUseTexture == 1)

    //gAlbedoSpec.rgb = albedo;
    //gAlbedoSpec.a = 1.0;

    gAlbedoSpec = vec4(0.9, 0.9, 0.9, 1.0);
    //gAlbedoSpec = vec4(1.0, 0.0, 0.0, 1.0);
}