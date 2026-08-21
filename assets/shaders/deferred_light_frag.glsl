#version 460

in vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D gPosition;
uniform sampler2D gNormal;
uniform sampler2D gAlbedoRoughness;
uniform sampler2D gMetallicAO;

uniform sampler2D u_ssaoMap;     // blurred AO factor from SSAOPass (R channel)
uniform int       u_ssaoEnabled; // toggle SSAO contribution at runtime

uniform sampler2D u_shadowMap;   // depth map from ShadowPass
uniform mat4      u_lightSpace;  // light-space view-projection matrix
uniform int       u_shadowEnabled;

uniform vec3 u_cameraPos;

struct GPULight {
    vec3  position;     float constant_att;
    vec3  ambient;      float linear_att;
    vec3  diffuse;      float quadratic_att;
    vec3  specular;     float cut_off;
    vec3  direction;    float outer_cut_off;
    vec3  pad;          int   type;
};

layout(std430, binding = 0) buffer LightBuffer {
    GPULight lights[];
};

uniform int lightCount;

const float PI = 3.14159265359;
const float INV_PI = 0.31830988618;

float sqr(float x) { return x * x; }

// PCF 3×3 shadow factor — returns 1.0 when fully lit, 0.0 when fully in shadow.
// Slope-scaled bias: surfaces nearly perpendicular to the light need a larger
// bias to avoid acne; surfaces facing the light directly can use a very small one.
float getShadowFactor(vec3 fragPos, vec3 N, vec3 lightDir) {
    vec4 fragLS     = u_lightSpace * vec4(fragPos, 1.0);
    vec3 proj       = fragLS.xyz / fragLS.w;
    proj            = proj * 0.5 + 0.5;
    if (proj.z > 1.0) return 1.0;
    float cosTheta  = max(dot(N, lightDir), 0.0);
    float bias      = max(0.001 * (1.0 - cosTheta), 0.0001);
    float shadow    = 0.0;
    vec2 texelSize  = 1.0 / textureSize(u_shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_shadowMap, proj.xy + vec2(x, y) * texelSize).r;
            shadow += (proj.z - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}

float schlickWeight(float cosTheta) {
    float m = clamp(1.0 - cosTheta, 0.0, 1.0);
    return m*m*m*m*m;
}

vec3 fresnelSchlick(vec3 F0, float cosTheta) {
    return F0 + (1.0 - F0) * schlickWeight(cosTheta);
}

float GTR2(float NdotH, float a) {
    float a2 = a * a;
    float t = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return a2 / (PI * t * t);
}

float smithG(float NdotV, float a) {
    float a2 = a * a;
    return 1.0 / (NdotV + sqrt(a2 + (1.0 - a2) * NdotV * NdotV));
}

vec3 evalDisney(
    vec3 baseColor,
    float metallic,
    float roughness,
    vec3 N,
    vec3 V,
    vec3 L
) {
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    vec3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);

    // F0
    vec3 F0 = mix(vec3(0.04), baseColor, metallic);
    vec3 F  = fresnelSchlick(F0, LdotH);

    float alpha = max(0.001, roughness * roughness);

    float D = GTR2(NdotH, alpha);
    float G = smithG(NdotL, alpha) * smithG(NdotV, alpha);

    vec3 spec = (F * D * G) / max(4.0 * NdotL * NdotV, 0.001);

    vec3 diff = baseColor * INV_PI * (1.0 - metallic);

    return (diff + spec) * NdotL;
}

vec3 evaluateLight(
    GPULight light,
    vec3 fragPos,
    vec3 N,
    vec3 V,
    vec3 albedo,
    float roughness,
    float metallic
) {
    vec3 L;
    float attenuation = 1.0;

    if (light.type == 0) {
        // DIRECTIONAL LIGHT
        L = normalize(-light.direction);
        attenuation = 1.0;
    }
    else if (light.type == 1) {
        // POINT LIGHT
        vec3 toLight = light.position - fragPos;
        float dist = length(toLight);
        L = normalize(toLight);

        float denom =
        light.constant_att +
        light.linear_att * dist +
        light.quadratic_att * dist * dist;

        attenuation = 1.0 / max(denom, 0.0001);
    }
    else {
        return vec3(0.0);
    }

    vec3 radiance = light.diffuse * attenuation;

    return evalDisney(albedo, metallic, roughness, N, V, L)
    * radiance;
}

void main()
{
    vec3 fragPos = texture(gPosition, v_texCoord).rgb;

    vec3 N = texture(gNormal, v_texCoord).rgb;
    if(length(N) < 0.001){
        discard;
    }
    N = normalize(N * 2.0 - 1.0);

    vec3 albedo = texture(gAlbedoRoughness, v_texCoord).rgb;
    float roughness = texture(gAlbedoRoughness, v_texCoord).a;
    float metallic  = texture(gMetallicAO, v_texCoord).r;

    vec3 V = normalize(u_cameraPos - fragPos);

    vec3 Lo = vec3(0.0);

    for (int i = 0; i < lightCount; i++) {
        vec3 contrib = evaluateLight(lights[i], fragPos, N, V, albedo, roughness, metallic);
        if (u_shadowEnabled != 0 && lights[i].type == 0) {
            vec3 L = normalize(-lights[i].direction);
            contrib *= getShadowFactor(fragPos, N, L);
        }
        Lo += contrib;
    }

    // Sample blurred AO map and attenuate ambient term.
    // When SSAO is disabled the factor is 1.0 (no change).
    float ao = (u_ssaoEnabled != 0) ? texture(u_ssaoMap, v_texCoord).r : 1.0;
    vec3 ambient = albedo * 0.12 * ao;

    vec3 color = ambient + Lo;

    color = color / (color + vec3(1.0));
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}