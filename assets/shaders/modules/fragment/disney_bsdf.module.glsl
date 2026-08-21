// MODULE_INFO
// name: disney_bsdf
// stage: fragment
// priority: 5
// dependencies: []
// description: Disney BSDF evaluation functions for direct lighting

// PARAMETERS

// FUNCTIONS
const float PI     = 3.14159265358979323846;
const float INV_PI = 0.31830988618379067154;

// --- Helpers ---

float sqr(float x) { return x * x; }

// Schlick Fresnel weight — F(0 deg) = 0, F(90 deg) = 1
float schlickWeight(float cosTheta) {
    float m = clamp(1.0 - cosTheta, 0.0, 1.0);
    return m * m * m * m * m;
}

// Full Schlick Fresnel between two values
float schlickFresnel(float u) {
    return schlickWeight(u);
}

vec3 schlickFresnelVec(vec3 F0, float cosTheta) {
    return F0 + (vec3(1.0) - F0) * schlickWeight(cosTheta);
}

// Generalized Trowbridge-Reitz — clearcoat uses this (gamma=1)
float GTR1(float NdotH, float a) {
    if (a >= 1.0) return INV_PI;
    float a2 = a * a;
    float t  = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return (a2 - 1.0) / (PI * log(a2) * t);
}

// Generalized Trowbridge-Reitz — specular uses this (gamma=2, isotropic)
float GTR2(float NdotH, float a) {
    float a2 = a * a;
    float t  = 1.0 + (a2 - 1.0) * NdotH * NdotH;
    return a2 / (PI * t * t);
}

// GTR2 anisotropic
float GTR2Aniso(float NdotH, float HdotX, float HdotY, float ax, float ay) {
    return 1.0 / (PI * ax * ay
        * sqr(sqr(HdotX / ax) + sqr(HdotY / ay) + NdotH * NdotH));
}

// Smith G for GGX — specular
float smithG_GGX(float NdotV, float alphaG) {
    float a = alphaG * alphaG;
    float b = NdotV  * NdotV;
    return 1.0 / (NdotV + sqrt(a + b - a * b));
}

// Smith G anisotropic
float smithG_GGXAniso(float NdotV, float VdotX, float VdotY, float ax, float ay) {
    return 1.0 / (NdotV + sqrt(sqr(VdotX * ax) + sqr(VdotY * ay) + sqr(NdotV)));
}

// --- Disney diffuse lobe ---
// Includes retro-reflection at grazing angles
vec3 evalDisneyDiffuse(vec3 baseColor, float roughness,
                       float NdotL, float NdotV, float LdotH) {
    float FL = schlickWeight(NdotL);
    float FV = schlickWeight(NdotV);

    // Retro-reflection
    float RR  = 2.0 * roughness * LdotH * LdotH;
    float FRetro = RR * (FL + FV + FL * FV * (RR - 1.0));

    float Fd = (1.0 - 0.5 * FL) * (1.0 - 0.5 * FV);

    return baseColor * INV_PI * (Fd + FRetro);
}

// --- Disney specular lobe ---
// Anisotropic GGX with Schlick Fresnel and tinting
vec3 evalDisneySpecular(
    vec3 baseColor,
    float metallic, float roughness,
    float specular, float specularTint,
    float anisotropic,
    float NdotL, float NdotV, float NdotH,
    float LdotH,
    float HdotX, float HdotY,
    float LdotX, float LdotY,
    float VdotX, float VdotY
) {
    float luminance = dot(baseColor, vec3(0.3, 0.6, 0.1));
    vec3  tint      = luminance > 0.0 ? baseColor / luminance : vec3(1.0);

    vec3 F0_dielectric = specular * 0.08 * mix(vec3(1.0), tint, specularTint);
    vec3 F0 = mix(F0_dielectric, baseColor, metallic);

    vec3 F = schlickFresnelVec(F0, LdotH);

    // --- Anisotropic distribution ---
    float aspect = sqrt(1.0 - 0.9 * anisotropic);
    float ax = max(0.001, sqr(roughness) / aspect);
    float ay = max(0.001, sqr(roughness) * aspect);

    float D = GTR2Aniso(NdotH, HdotX, HdotY, ax, ay);
    
    float G =
    smithG_GGXAniso(NdotL, LdotX, LdotY, ax, ay) *
    smithG_GGXAniso(NdotV, VdotX, VdotY, ax, ay);
    
    return (F * D * G) / max(4.0 * NdotL * NdotV, 0.001);
}

// --- Disney clearcoat lobe ---
float evalDisneyClearcoat(
    float clearcoatGloss,
    float NdotL, float NdotV,
    float NdotH, float LdotH
) {
    float gloss = mix(0.1, 0.001, clearcoatGloss);

    float D = GTR1(NdotH, gloss);
    float F = mix(0.04, 1.0, schlickWeight(LdotH));
    float G = smithG_GGX(NdotL, 0.25) * smithG_GGX(NdotV, 0.25);
    
    return (D * F * G) / max(4.0 * NdotL * NdotV, 0.001);
}

// --- Disney sheen lobe ---
// Adds edge brightness — useful for cloth and fabric
vec3 evalDisneySheen(vec3 baseColor, float sheen, float sheenTint, float LdotH) {
    float luminance = dot(baseColor, vec3(0.3, 0.6, 0.1));
    vec3  tint      = luminance > 0.0 ? baseColor / luminance : vec3(1.0);
    vec3  sheenColor = mix(vec3(1.0), tint, sheenTint);
    return sheen * sheenColor * schlickWeight(LdotH);
}

// --- Full Disney BSDF evaluation ---
// All lobes combined. Returns radiance contribution for one light.
vec3 evalDisney(
    vec3 baseColor,
    float metallic,
    float roughness,
    float specular,
    float specularTint,
    float anisotropic,
    float sheen,
    float sheenTint,
    float clearcoat,
    float clearcoatGloss,
    vec3 N,
    vec3 V,
    vec3 L,
    vec3 X,
    vec3 Y
) {
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.0);
    if (NdotL <= 0.0 || NdotV <= 0.0) return vec3(0.0);

    vec3 H = normalize(L + V);

    float NdotH = max(dot(N, H), 0.0);
    float LdotH = max(dot(L, H), 0.0);
    
    float HdotX = dot(H, X);
    float HdotY = dot(H, Y);

    float LdotX = dot(L, X);
    float LdotY = dot(L, Y);

    float VdotX = dot(V, X);
    float VdotY = dot(V, Y);
    
    vec3 diffuse = evalDisneyDiffuse(baseColor, roughness,
                                     NdotL, NdotV, LdotH)
    * (1.0 - metallic);
    
    vec3 sheenContrib = evalDisneySheen(baseColor, sheen, sheenTint, LdotH)
    * (1.0 - metallic);
    
    vec3 spec = evalDisneySpecular(
        baseColor, metallic, roughness,
        specular, specularTint, anisotropic,
        NdotL, NdotV, NdotH, LdotH,
        HdotX, HdotY,
        LdotX, LdotY,
        VdotX, VdotY
    );
    
    float coat = 0.25 * clearcoat *
    evalDisneyClearcoat(clearcoatGloss,
                        NdotL, NdotV, NdotH, LdotH);
    
    return (diffuse + sheenContrib + spec) * NdotL
    + vec3(coat * NdotL);
}

// MAIN_CODE