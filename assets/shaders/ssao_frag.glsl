#version 460

in  vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D gPosition;       // world-space positions (RGBA32F)
uniform sampler2D gNormal;         // world-space normals   (RGBA16F, encoded x*0.5+0.5)
uniform sampler2D u_noiseTexture;  // 4x4 random RG rotation vectors (RG32F, REPEAT)

uniform vec3  u_samples[64];       // hemisphere kernel in tangent space
uniform mat4  u_projection;        // camera projection matrix
uniform mat4  u_view;              // camera view matrix
uniform vec2  u_noiseScale;        // viewport / 4 — how many times the noise tile repeats

const int   KERNEL_SIZE = 64;
const float RADIUS      = 0.5;     // hemisphere radius in view space (world units)
const float BIAS        = 0.025;   // depth bias to prevent self-occlusion acne

void main()
{
    // ── Detect sky / cleared fragments via raw (un-decoded) normal ──────────
    // GeometryPass clears to (0,0,0,1); rendered normals encode to [0,1] range
    // so their RGB will never be exactly zero.
    vec3 normalRaw = texture(gNormal, v_texCoord).rgb;
    if (length(normalRaw) < 0.001) { FragColor = vec4(1.0); return; }

    // ── Read G-Buffer ────────────────────────────────────────────────────────
    vec3 fragPosWorld = texture(gPosition, v_texCoord).rgb;
    vec3 normalWorld  = normalize(normalRaw * 2.0 - 1.0);

    // Random rotation vector from the tiling 4x4 noise texture.
    // The Z component is 0 — rotation stays in the tangent plane.
    vec3 randomVec = vec3(texture(u_noiseTexture, v_texCoord * u_noiseScale).rg, 0.0);

    // ── Transform to view space ──────────────────────────────────────────────
    // Positions need a full 4x4 multiply; normals only need the rotation part.
    // For an orthonormal (no-scale) view matrix, mat3(u_view) == normal matrix.
    vec3 fragPosView = vec3(u_view * vec4(fragPosWorld, 1.0));
    vec3 normalView  = normalize(mat3(u_view) * normalWorld);

    // ── Build TBN matrix (Gram-Schmidt orthogonalisation) ────────────────────
    // Re-orthogonalise tangent against normal so TBN is a proper basis.
    vec3 tangent   = normalize(randomVec - normalView * dot(randomVec, normalView));
    vec3 bitangent = cross(normalView, tangent);
    mat3 TBN       = mat3(tangent, bitangent, normalView);

    // ── Sample hemisphere and accumulate occlusion ───────────────────────────
    float occlusion = 0.0;

    for (int i = 0; i < KERNEL_SIZE; ++i)
    {
        // Rotate kernel sample into the fragment's view-space hemisphere
        vec3 samplePos = fragPosView + TBN * u_samples[i] * RADIUS;

        // Project sample position to get its screen-space UV
        vec4 projected  = u_projection * vec4(samplePos, 1.0);
        projected.xyz  /= projected.w;              // perspective divide
        vec2 sampleUV   = projected.xy * 0.5 + 0.5; // NDC [-1,1] -> UV [0,1]

        // Skip samples that fall outside the viewport
        if (sampleUV.x < 0.0 || sampleUV.x > 1.0 ||
            sampleUV.y < 0.0 || sampleUV.y > 1.0)
            continue;

        // Get view-space Z of the actual geometry at this UV
        vec3  geoWorld  = texture(gPosition, sampleUV).rgb;
        float geoViewZ  = (u_view * vec4(geoWorld, 1.0)).z;

        // Range check — attenuate contribution of surfaces far from the fragment
        // to prevent a dark halo around objects against distant backgrounds.
        // In right-handed view space Z is negative; abs handles both signs.
        float rangeCheck = smoothstep(0.0, 1.0, RADIUS / abs(fragPosView.z - geoViewZ));

        // Occluded if the geometry is closer to the camera than the sample point.
        // In right-handed view space: larger Z == closer to camera.
        // geoViewZ > samplePos.z means geometry is in front of the sample -> occluded.
        occlusion += (geoViewZ > samplePos.z + BIAS ? 1.0 : 0.0) * rangeCheck;
    }

    // Invert so 0.0 == fully occluded, 1.0 == fully unoccluded
    float ao = 1.0 - (occlusion / float(KERNEL_SIZE));
    FragColor = vec4(ao, ao, ao, 1.0);
}
