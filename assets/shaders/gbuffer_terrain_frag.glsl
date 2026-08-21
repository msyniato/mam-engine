#version 460

in vec3 v_fragPos;
in vec3 v_normal;
in vec2 v_texCoord;
in vec4 v_tangent;
in vec2 v_worldXZ;
in float v_worldY;
in vec3 v_worldNormal;

// GBuffer outputs
layout(location = 0) out vec4 gPosition;
layout(location = 1) out vec4 gNormal;
layout(location = 2) out vec4 gAlbedoRoughness;
layout(location = 3) out vec4 gMetallicAO;

// Terrain splat textures
uniform sampler2D u_texture0; // base (grass/ground)
uniform sampler2D u_texture1; // slope (rock/cliff)
uniform sampler2D u_texture2; // high (snow/peak)
uniform sampler2D u_texture3; // detail overlay

// Splat parameters
uniform float u_slopeMin        = 0.08;
uniform float u_slopeMax        = 0.35;
uniform float u_heightLow       = 180.0;
uniform float u_heightHigh      = 360.0;
uniform float u_texScale        = 0.02;
uniform float u_detailScale     = 0.5;
uniform float u_detailStrength  = 0.35;
uniform float u_detailFadeStart = 60.0;
uniform float u_detailFadeEnd   = 280.0;

vec3 sampleTerrain(sampler2D tex, vec2 worldXZ, float scale) {
    return texture(tex, worldXZ * scale).rgb;
}

void main()
{
    float slope      = 1.0 - v_worldNormal.y;
    float slopeBlend = smoothstep(u_slopeMin, u_slopeMax, slope);
    float heightBlend = smoothstep(u_heightLow, u_heightHigh, v_worldY);

    vec3 color0 = sampleTerrain(u_texture0, v_worldXZ, u_texScale);
    vec3 color1 = sampleTerrain(u_texture1, v_worldXZ, u_texScale);
    vec3 color2 = sampleTerrain(u_texture2, v_worldXZ, u_texScale);

    vec3 albedo = mix(color0, color1, slopeBlend);
         albedo = mix(albedo, color2, heightBlend);

    vec3  detail     = texture(u_texture3, v_worldXZ * u_detailScale).rgb;
    float viewDist   = 1.0 / gl_FragCoord.w;
    float detailFade = 1.0 - smoothstep(u_detailFadeStart, u_detailFadeEnd, viewDist);
    float weight     = u_detailStrength * detailFade;

    albedo += (detail - vec3(0.5)) * 2.0 * weight;
    albedo  = clamp(albedo, 0.0, 1.0);

    vec3 N = normalize(v_worldNormal);

    gPosition        = vec4(v_fragPos, 1.0);
    gNormal          = vec4(N * 0.5 + 0.5, 1.0);
    gAlbedoRoughness = vec4(albedo, 0.9);
    gMetallicAO      = vec4(0.0, 1.0, 0.0, 1.0);
}
