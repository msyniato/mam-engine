// name: shadow_receiver
// stage: fragment
// dependencies: []
// priority: 10

// PARAMETERS
// uniform mat4 u_lightSpace
// uniform sampler2D u_shadowMap

// FUNCTIONS
float getShadowFactor(vec3 fragPos, vec3 normal, vec3 lightDir) {
    vec4 fragPosLightSpace = u_lightSpace * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0) return 1.0;

    float closestDepth = texture(u_shadowMap, projCoords.xy).r;
    float currentDepth = projCoords.z;
    //float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.001);
    float bias = 0.02;
    
    // PCF - soft shadows, sample 3x3 kernel
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_shadowMap, 0);
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(u_shadowMap,
                projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += (currentDepth - bias) > pcfDepth ? 0.0 : 1.0;
        }
    }
    return shadow / 9.0;
}
