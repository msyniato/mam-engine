// MODULE_INFO
// name: pbr_surface
// stage: fragment
// priority: 20
// dependencies: [pbr_lighting]
// description: Disney BSDF surface — reads PBR textures and calls calcPBRLighting

// PARAMETERS
// uniform sampler2D u_texture0        : null
// uniform sampler2D u_texture1        : null
// uniform sampler2D u_texture2        : null
// uniform sampler2D u_texture3        : null
// uniform int       u_useAlbedoMap     : 0
// uniform int       u_useNormalMap     : 0
// uniform int       u_useRoughnessMap  : 0
// uniform int       u_useMetallicMap   : 0
// uniform vec3      u_baseColor       : [1.0, 1.0, 1.0]
// uniform float     u_metallic        : 0.0
// uniform float     u_roughness       : 0.5
// uniform float     u_specular        : 0.5
// uniform float     u_specularTint    : 0.0
// uniform float     u_anisotropic     : 0.0
// uniform float     u_sheen           : 0.0
// uniform float     u_sheenTint       : 0.5
// uniform float     u_clearcoat       : 0.0
// uniform float     u_clearcoatGloss  : 1.0
// uniform vec3      u_sh[9]           : []
// uniform int       u_probeEnabled    : 0
// input   vec2      v_texCoord
// input   vec3      v_normal
// input   vec3      v_fragPos
// input   vec4      v_tangent

// FUNCTIONS

// MAIN_CODE
// --- Sample textures ---
vec3  baseColor = u_baseColor;
float metallic  = u_metallic;
float roughness = u_roughness;

if (u_useAlbedoMap != 0) {
    vec4 albedoSample = texture(u_texture0, v_texCoord);
    baseColor = pow(albedoSample.rgb, vec3(2.2)); // sRGB to linear
}

if (u_useRoughnessMap != 0) {
    roughness = texture(u_texture2, v_texCoord).r;
    roughness = clamp(roughness, 0.04, 1.0);
}

if (u_useMetallicMap != 0) {
    metallic = texture(u_texture3, v_texCoord).r;
}



// --- Build TBN ---
vec3 N = normalize(v_normal);
vec3 T;
vec3 B;

vec3 tangent3 = v_tangent.xyz * v_tangent.w;

if (length(tangent3) > 0.01) {
    T = normalize(tangent3 - N * dot(N, tangent3));
    B = normalize(cross(N, T));
} else {
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// --- Normal map ---
if (u_useNormalMap != 0) {
    vec3 tangentNormal = texture(u_texture1, v_texCoord).rgb * 2.0 - 1.0;
    tangentNormal.xy *= 0.3;  
    tangentNormal = normalize(tangentNormal);
    mat3 TBN = mat3(T, B, N);
    N = normalize(TBN * tangentNormal);
}

// --- Lighting ---
vec3 lit = calcPBRLighting(
    baseColor,
    metallic,
    roughness,
    u_specular,
    u_specularTint,
    u_anisotropic,
    u_sheen,
    u_sheenTint,
    u_clearcoat,
    u_clearcoatGloss,
    N,
    v_fragPos,
    tangent3
);

// --- Tonemap + gamma correct ---
vec3 color = lit / (lit + vec3(1.0));
color = pow(color, vec3(1.0 / 2.2));

FragColor = vec4(color, 1.0);