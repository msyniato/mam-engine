// MODULE_INFO
// name: terrain_splatting
// stage: fragment
// dependencies: []
// priority: 10
// description: Splat-blended terrain with high-frequency detail map

// PARAMETERS
// uniform sampler2D u_texture0
// uniform sampler2D u_texture1
// uniform sampler2D u_texture2
// uniform sampler2D u_texture3
// uniform float u_slopeMin : 0.08
// uniform float u_slopeMax : 0.35
// uniform float u_heightLow : 180.0
// uniform float u_heightHigh : 360.0
// uniform float u_texScale : 0.02
// uniform float u_detailScale : 0.5
// uniform float u_detailStrength : 0.35
// uniform float u_detailFadeStart : 60.0
// uniform float u_detailFadeEnd : 280.0
// input vec2 v_worldXZ
// input float v_worldY
// input vec3 v_worldNormal

// FUNCTIONS
vec3 sampleTerrain(sampler2D tex, vec2 worldXZ, float scale) {
	return texture(tex, worldXZ * scale).rgb;
}

// MAIN_CODE
float slope      = 1.0 - v_worldNormal.y;
float slopeBlend  = smoothstep(u_slopeMin, u_slopeMax, slope);
float heightBlend = smoothstep(u_heightLow, u_heightHigh, v_worldY);

vec3 color0 = sampleTerrain(u_texture0, v_worldXZ, u_texScale);
vec3 color1 = sampleTerrain(u_texture1, v_worldXZ, u_texScale);
vec3 color2 = sampleTerrain(u_texture2, v_worldXZ, u_texScale);

vec3 color = mix(color0, color1, slopeBlend);
     color = mix(color, color2, heightBlend);

vec3 detail = texture(u_texture3, v_worldXZ * u_detailScale).rgb;
float viewDist   = 1.0 / gl_FragCoord.w;
float detailFade = 1.0 - smoothstep(u_detailFadeStart, u_detailFadeEnd, viewDist);
float weight     = u_detailStrength * detailFade;

color += (detail - vec3(0.5)) * 2.0 * weight;
color  = clamp(color, 0.0, 1.0);

FragColor = vec4(color, 1.0);