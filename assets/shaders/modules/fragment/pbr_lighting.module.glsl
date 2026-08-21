// MODULE_INFO
// name: pbr_lighting
// stage: fragment
// priority: 10
// dependencies: [disney_bsdf]
// description: Direct lighting loop using Disney BSDF

// PARAMETERS
// uniform int  u_lightCount : 0
// uniform vec3 u_viewPos    : [0.0, 0.0, 0.0]
// uniform vec3 u_sh[9]        : []
// uniform int  u_probeEnabled : 0
// input   vec3 v_normal
// input   vec3 v_fragPos
// input   vec4 v_tangent

// FUNCTIONS
struct GPULight {
    vec3  position;     float constant_att;
    vec3  ambient;      float linear_att;
    vec3  diffuse;      float quadratic_att;
    vec3  specular;     float cut_off;
    vec3  direction;    float outer_cut_off;
    vec3  pad;          int   type;        
};

layout(std430, binding = 0) readonly buffer LightBuffer {
    GPULight lights[];
};

// Evaluate L2 SH irradiance for a surface normal
// Coefficients match the projection in light_probe.cpp (Ramamoorthi & Hanrahan)
vec3 evaluateProbeIrradiance(vec3 n) {
    float x = n.x, y = n.y, z = n.z;

    const float c0 = 0.282095 * 3.141593;   // l=0
    const float c1 = 0.488603 * 2.094395;   // l=1
    const float c2 = 1.092548 * 0.785398;   // l=2 cross terms
    const float c3 = 0.315392 * 0.785398;   // l=2 Y6
    const float c4 = 0.546274 * 0.785398;   // l=2 Y8

    vec3 irr = vec3(0.0);
    irr += u_sh[0] * c0;
    irr += u_sh[1] * (c1 * y);
    irr += u_sh[2] * (c1 * z);
    irr += u_sh[3] * (c1 * x);
    irr += u_sh[4] * (c2 * x * y);
    irr += u_sh[5] * (c2 * y * z);
    irr += u_sh[6] * (c3 * (3.0 * z * z - 1.0));
    irr += u_sh[7] * (c2 * x * z);
    irr += u_sh[8] * (c4 * (x * x - y * y));
    return max(irr, vec3(0.0));
}


float calcAttenuation(GPULight light, float dist) {
    return 1.0 / (light.constant_att
                + light.linear_att    * dist
                + light.quadratic_att * dist * dist);
}

// Builds an arbitrary tangent and bitangent from the normal.
// Used when no mesh tangent is available.
void buildTBN(vec3 N, out vec3 T, out vec3 B) {
    vec3 up = abs(N.y) < 0.999 ? vec3(0, 1, 0) : vec3(1, 0, 0);
    T = normalize(cross(up, N));
    B = cross(N, T);
}

// Calls evalDisney for a single light direction and color
vec3 shadeSingleLight(
    vec3 lightDir, vec3 lightColor,
    vec3 baseColor, float metallic, float roughness,
    float specular, float specularTint, float anisotropic,
    float sheen, float sheenTint,
    float clearcoat, float clearcoatGloss,
    vec3 N, vec3 V, vec3 T, vec3 B
) {
    return evalDisney(
        baseColor, metallic, roughness,
        specular, specularTint, anisotropic,
        sheen, sheenTint, clearcoat, clearcoatGloss,
        N, V, lightDir, T, B
    ) * lightColor;
}

// Main entry — called by pbr_surface
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
    vec3  inTangent   // mesh tangent, may be zero
) {
    vec3 V = normalize(u_viewPos - fragPos);

    // Build TBN — use mesh tangent if provided, else derive from normal
    vec3 T, B;
    if (length(inTangent) > 0.01) {
        T = normalize(inTangent - dot(inTangent, N) * N);
        B = cross(N, T);
    } else {
        buildTBN(N, T, B);
    }

    vec3 result = vec3(0.0);

    for (int i = 0; i < u_lightCount; i++) {
        GPULight light = lights[i];
        vec3 lightDir;
        vec3 lightColor;
        float attenuation = 1.0;

        if (light.type == 0) {
            // Directional
            lightDir   = normalize(-light.direction);
            lightColor = light.diffuse;

        } else if (light.type == 1) {
            // Point
            vec3  toLight = light.position - fragPos;
            float dist    = length(toLight);
            lightDir      = normalize(toLight);
            attenuation   = calcAttenuation(light, dist);
            lightColor    = light.diffuse * attenuation;

        } else if (light.type == 2) {
            // Spot
            vec3  toLight  = light.position - fragPos;
            float dist     = length(toLight);
            lightDir       = normalize(toLight);
            float theta    = dot(lightDir, normalize(-light.direction));
            float epsilon  = max(light.cut_off - light.outer_cut_off, 0.001);
            float spotMask = clamp((theta - light.outer_cut_off) / epsilon, 
                                0.0, 1.0);
            attenuation    = calcAttenuation(light, dist) * spotMask;
            lightColor     = light.diffuse * attenuation;
        }
        
        result += shadeSingleLight(
            lightDir, lightColor,
            baseColor, metallic, roughness,
            specular, specularTint, anisotropic,
            sheen, sheenTint, clearcoat, clearcoatGloss,
            N, V, T, B
        );

        // Ambient contribution from each light — keeps shadowed areas visible
        if (u_probeEnabled == 0) {
            result += baseColor * light.ambient * (1.0 - metallic);
        }
    }

    if (u_probeEnabled != 0) {
        vec3 probeIrradiance = evaluateProbeIrradiance(N);

        result += baseColor * probeIrradiance * 2.5 * (1.0 - metallic);
    }

    return result;
}

// MAIN_CODE