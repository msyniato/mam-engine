#version 460

in vec2 v_texCoord;
in vec3 v_fragPos;
in vec3 v_normal;
in vec4 v_tangent;

// GBuffer outputs
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoRoughness;
layout(location = 3) out vec4 gMetallicAO;

// Textures
uniform sampler2D u_albedoMap;
uniform sampler2D u_normalMap;
uniform sampler2D u_roughnessMap;
uniform sampler2D u_metallicMap;

// Flags
uniform int u_useAlbedoMap;
uniform int u_useNormalMap;
uniform int u_useRoughnessMap;
uniform int u_useMetallicMap;

void main()
{
    vec3 albedo = vec3(1.0);
    if (u_useAlbedoMap != 0)
    albedo = texture(u_albedoMap, v_texCoord).rgb;
    
    vec3 N = normalize(v_normal);

    vec3 T = normalize(vec3(v_tangent));
    T = normalize(T - dot(T, N) * N);

    vec3 B = cross(N, T) * v_tangent.w;

    mat3 TBN = mat3(T, B, N);

    vec3 worldNormal = N;

    if (u_useNormalMap != 0)
    {
        vec3 normalTS = texture(u_normalMap, v_texCoord).rgb;
        normalTS = normalTS * 2.0 - 1.0;
        worldNormal = normalize(TBN * normalTS);
    }
    
    float roughness = 1.0;
    if (u_useRoughnessMap != 0)
    roughness = texture(u_roughnessMap, v_texCoord).r;
    
    float metallic = 0.0;
    if (u_useMetallicMap != 0)
    metallic = texture(u_metallicMap, v_texCoord).r;
    
    gPosition = vec4(v_fragPos, 1.0);

    gNormal = vec4(worldNormal * 0.5 + 0.5, 1.0);

    gAlbedoRoughness = vec4(albedo, roughness);

    gMetallicAO = vec4(metallic, 1.0, 0.0, 1.0);
}