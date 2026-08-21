// MODULE_INFO
// name: vegetation_frag
// stage: fragment
// dependencies: []
// priority: 10

// PARAMETERS
// uniform float u_ambientStr : 0.45
// input vec3 v_normal
// input vec3 v_worldPos
// input vec2 v_texCoord

// FUNCTIONS
vec3 trunkColor() {
    return vec3(0.30, 0.18, 0.10);   // marrón
}

vec3 leafColor(vec3 wp) {
    // verde con variación según worldPos (cada árbol un tono distinto)
    float t = fract(sin(dot(floor(wp.xz * 0.01), vec2(12.9898, 78.233))) * 43758.5453);
    vec3 c1 = vec3(0.18, 0.55, 0.12);   // verde primavera
    vec3 c2 = vec3(0.10, 0.40, 0.08);   // verde oscuro
    vec3 c3 = vec3(0.32, 0.55, 0.10);   // verde amarillento
    if (t < 0.33)      return c1;
    else if (t < 0.66) return c2;
    else               return c3;
}

// MAIN_CODE
vec3 N = normalize(v_normal);
float leafiness = abs(N.y);              // 0 = tronco, 1 = hoja muy plana
vec3 baseColor  = mix(trunkColor(), leafColor(v_worldPos), smoothstep(0.2, 0.6, leafiness));

vec3 lightDir = normalize(vec3(0.4, 1.0, 0.3));
float diff    = max(dot(N, lightDir), 0.0);
float wrap    = max(dot(N, lightDir) * 0.5 + 0.5, 0.0);   // wrap lighting suaviza
float light   = u_ambientStr + (1.0 - u_ambientStr) * mix(diff, wrap, 0.7);

FragColor = vec4(baseColor * light, 1.0);