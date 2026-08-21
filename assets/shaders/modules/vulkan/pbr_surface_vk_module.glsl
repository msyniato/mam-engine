// name: pbr_surface_vk
// stage: fragment
// dependencies: [pbr_lighting_vk]
// priority: 20
// description: Disney BSDF surface — lee texturas PBR y llama a calcPBRLighting (Vulkan).
//              Texturas en set=0 bindings=1-4. UBO de material en set=0 binding=0.

// PARAMETERS
// uniform sampler2D u_texture0       : null   (albedo,    set=0 binding=1)
// uniform sampler2D u_texture1       : null   (normal,    set=0 binding=2)
// uniform sampler2D u_texture2       : null   (roughness, set=0 binding=3)
// uniform sampler2D u_texture3       : null   (metallic,  set=0 binding=4)
// uniform int       u_useAlbedoMap    : 0
// uniform int       u_useNormalMap    : 0
// uniform int       u_useRoughnessMap : 0
// uniform int       u_useMetallicMap  : 0
// uniform vec3      u_baseColor      : [1.0, 1.0, 1.0]
// uniform float     u_metallic       : 0.0
// uniform float     u_roughness      : 0.5
// uniform float     u_specular       : 0.5
// uniform float     u_specularTint   : 0.0
// uniform float     u_anisotropic    : 0.0
// uniform float     u_sheen          : 0.0
// uniform float     u_sheenTint      : 0.5
// uniform float     u_clearcoat      : 0.0
// uniform float     u_clearcoatGloss : 1.0
// input   vec2      v_texCoord
// input   vec3      v_normal
// input   vec3      v_fragPos
// input   vec4      v_tangent
// output  vec4      outColor

// FUNCTIONS

vec3 vk_acesFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 vk_linearToSRGB(vec3 color)
{
    return pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));
}

// MAIN_CODE

// --- Leer textures ---
vec3  baseColor = u_baseColor;
float metallic  = u_metallic;
float roughness = u_roughness;

if (u_useAlbedoMap != 0) {
    vec4 albedoSample = texture(u_texture0, v_texCoord);
    baseColor = pow(albedoSample.rgb, vec3(2.2)); // sRGB → linear
}

if (u_useRoughnessMap != 0) {
    roughness = clamp(texture(u_texture2, v_texCoord).r, 0.04, 1.0);
}

if (u_useMetallicMap != 0) {
    metallic = texture(u_texture3, v_texCoord).r;
}

// --- Construir TBN ---
vec3 N = normalize(v_normal);
vec3 T_surf, B_surf;

vec3 tangent3 = v_tangent.xyz * v_tangent.w;

if (length(tangent3) > 0.01) {
    T_surf = normalize(tangent3 - N * dot(N, tangent3));
    B_surf = normalize(cross(N, T_surf));
} else {
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T_surf  = normalize(cross(up, N));
    B_surf  = cross(N, T_surf);
}

// --- Normal map ---
if (u_useNormalMap != 0) {
    vec3 tangentNormal = texture(u_texture1, v_texCoord).rgb * 2.0 - 1.0;
    tangentNormal.xy  *= 0.3;
    tangentNormal      = normalize(tangentNormal);
    N = normalize(mat3(T_surf, B_surf, N) * tangentNormal);
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

// --- Tonemapping Reinhard + gamma ---
vec3 color = vk_acesFilm(max(lit, vec3(0.0)));
color = vk_linearToSRGB(color);

outColor = vec4(color, 1.0);
