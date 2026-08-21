// MODULE_INFO
// name: lighting
// stage: fragment
// priority: 10
// dependencies: []
// description: Blinn-Phong lighting — directional, point and spot lights from SSBO

// PARAMETERS
// uniform int  u_lightCount : 0
// uniform vec3 u_viewPos    : [0.0, 0.0, 0.0]
// input   vec3 v_normal
// input   vec3 v_fragPos

// FUNCTIONS
struct GPULight {
    vec3  position;     float constant_att;
    vec3  ambient;      float linear_att;
    vec3  diffuse;      float quadratic_att;
    vec3  specular;     float cut_off;
    vec3  direction;    float outer_cut_off;
    int   type;         vec3  pad;
};

layout(std430, binding = 0) readonly buffer LightBuffer {
    GPULight lights[];
};

// Shared attenuation formula used by point and spot lights
float calcAttenuation(GPULight light, float dist) {
    return 1.0 / (light.constant_att
    + light.linear_att    * dist
    + light.quadratic_att * dist * dist);
}

// Shared diffuse + specular term used by all three types
vec3 calcBlinnPhong(GPULight light, vec3 lightDir, vec3 normal, vec3 viewDir) {
    // diffuse
    float diff = max(dot(normal, lightDir), 0.0);

    // specular — Blinn-Phong halfway vector
    vec3  halfway = normalize(lightDir + viewDir);
    float spec    = pow(max(dot(normal, halfway), 0.0), 32.0);

    return light.ambient
    + light.diffuse  * diff
    + light.specular * spec;
}

// type == 0
vec3 calcDirectionalLight(GPULight light, vec3 normal, vec3 viewDir) {
    // direction points away from the surface, so negate the stored direction
    vec3 lightDir = normalize(-light.direction);
    return calcBlinnPhong(light, lightDir, normal, viewDir);
    // no attenuation — directional lights are infinitely far away
}

// type == 1
vec3 calcPointLight(GPULight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3  toLight   = light.position - fragPos;
    vec3  lightDir  = normalize(toLight);
    float dist      = length(toLight);
    float att       = calcAttenuation(light, dist);

    return calcBlinnPhong(light, lightDir, normal, viewDir) * att;
}

// type == 2
vec3 calcSpotLight(GPULight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3  toLight  = light.position - fragPos;
    vec3  lightDir = normalize(toLight);
    float dist     = length(toLight);

    // angle between the light direction and the direction to the fragment
    float theta    = dot(lightDir, normalize(-light.direction));

    // cut_off and outer_cut_off are stored as cosines of the angles
    // theta > cut_off means inside the inner cone
    float epsilon  = light.cut_off - light.outer_cut_off;
    float intensity = clamp((theta - light.outer_cut_off) / epsilon, 0.0, 1.0);

    float att = calcAttenuation(light, dist);

    return calcBlinnPhong(light, lightDir, normal, viewDir) * att * intensity;
}

// Entry point — called by the texture/material modules
vec3 calcLighting(vec3 albedo, vec3 normal, vec3 fragPos) {
    vec3 viewDir = normalize(u_viewPos - fragPos);
    vec3 result  = vec3(0.0);

    for (int i = 0; i < u_lightCount; i++) {
        if      (lights[i].type == 0) result += calcDirectionalLight(lights[i], normal, viewDir);
        else if (lights[i].type == 1) result += calcPointLight(lights[i], normal, fragPos, viewDir);
        else if (lights[i].type == 2) result += calcSpotLight(lights[i], normal, fragPos, viewDir);
    }

    return albedo * result;
}

// MAIN_CODE
