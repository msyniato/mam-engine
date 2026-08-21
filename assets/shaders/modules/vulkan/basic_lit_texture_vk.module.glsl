// name: basic_lit_texture_vk
// stage: fragment
// dependencies: []
// priority: 0
// description: Vulkan simple directional lighting with u_texture0.

// PARAMETERS
// uniform sampler2D u_texture0
// input vec3 v_normal
// input vec2 v_texCoord
// output vec4 outColor

// FUNCTIONS

// MAIN_CODE
vec3 baseColor = texture(u_texture0, v_texCoord).rgb;
vec3 n = normalize(v_normal);
vec3 lightDir = normalize(vec3(-0.35, -1.0, -0.25));
float ndotl = max(dot(n, -lightDir), 0.0);
vec3 ambient = baseColor * 0.20;
vec3 diffuse = baseColor * ndotl * 0.80;
outColor = vec4(ambient + diffuse, 1.0);
