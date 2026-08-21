// name: pbr_lighting_vk
// stage: fragment
// dependencies: [disney_bsdf_vk]
// priority: 10
// description: Direct lighting loop using Disney BSDF (Vulkan). LightBuffer en set=1 binding=0.

// PARAMETERS
// uniform vec3  u_viewPos   : [0.0, 0.0, 0.0]   (via UBO, set=0)
// uniform int   u_lightCount : 0                  (via UBO, set=0)
// input   vec3  v_fragPos
// input   vec3  v_normal
// input   vec4  v_tangent

// FUNCTIONS
struct GPULight {
    vec3  position;      float constant_att;
    vec3  ambient;       float linear_att;
    vec3  diffuse;       float quadratic_att;
    vec3  specular;      float cut_off;
    vec3  direction;     float outer_cut_off;
    vec3  pad;           int   type;
};

// Vulkan: descriptor set 1, binding 0 — light SSBO
layout(set = 1, binding = 0) readonly buffer LightBuffer {
    GPULight lights[];
};

float vk_calcAttenuation(GPULight light, float dist) {
    return 1.0 / (light.constant_att
                + light.linear_att    * dist
                + light.quadratic_att * dist * dist);
}

void vk_buildTBN(vec3 N, out vec3 T, out vec3 B) {
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(1.0, 0.0, 0.0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

vec3 calcPBRLighting(
    vec3  baseColor,
    float metallic,
    float roughness,
    float specular,
    float specularTint,
    float anisotropic,
    float sheen,
    float sheenTint,
    float clearcoat,
    float clearcoatGloss,
    vec3  N,
    vec3  fragPos,
    vec3  inTangent
) {
    vec3 V = normalize(u_viewPos - fragPos);

    vec3 T, B;
    if (length(inTangent) > 0.01) {
        T = normalize(inTangent - dot(inTangent, N) * N);
        B = cross(N, T);
    } else {
        vk_buildTBN(N, T, B);
    }

    vec3 result = vec3(0.0);

    for (int i = 0; i < u_lightCount; i++) {
        GPULight light = lights[i];
        vec3 lightDir = vec3(0.0, 1.0, 0.0);
        vec3 lightColor = vec3(0.0);

        if (light.type == 0) {
            lightDir   = normalize(-light.direction);
            lightColor = light.diffuse;

        } else if (light.type == 1) {
            vec3  toLight = light.position - fragPos;
            float dist    = length(toLight);
            lightDir      = normalize(toLight);
            lightColor    = light.diffuse * vk_calcAttenuation(light, dist);

        } else if (light.type == 2) {
            vec3  toLight  = light.position - fragPos;
            float dist     = length(toLight);
            lightDir       = normalize(toLight);
            float theta    = dot(lightDir, normalize(-light.direction));
            float epsilon  = max(light.cut_off - light.outer_cut_off, 0.0001);
            float spotMask = clamp((theta - light.outer_cut_off) / epsilon, 0.0, 1.0);
            lightColor     = light.diffuse * vk_calcAttenuation(light, dist) * spotMask;

        } else {
            continue;
        }

        result += evalDisney(
            baseColor, metallic, roughness,
            specular, specularTint, anisotropic,
            sheen, sheenTint, clearcoat, clearcoatGloss,
            N, V, lightDir, T, B
        ) * lightColor;

        // Ambient (apagado para metálicos)
        result += baseColor * light.ambient * (1.0 - metallic);
    }

    return result;
}

// MAIN_CODE
