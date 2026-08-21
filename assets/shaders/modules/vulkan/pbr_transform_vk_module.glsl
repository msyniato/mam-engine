// name: pbr_transform_vk
// stage: vertex
// dependencies: []
// priority: 0
// description: Vulkan vertex transform con tangente para PBR. Extiende basic_transform_vk.

// PARAMETERS
// uniform mat4 u_model
// uniform mat4 u_view
// uniform mat4 u_projection
// input   vec3 a_position
// input   vec3 a_normal
// input   vec2 a_texCoord
// input   vec4 a_tangent     (xyz = tangent, w = handedness)
// output  vec3 v_fragPos
// output  vec3 v_normal
// output  vec2 v_texCoord
// output  vec4 v_tangent

// FUNCTIONS

// MAIN_CODE
mat3 normalMatrix = mat3(transpose(inverse(u_model)));

vec4 worldPos = u_model * vec4(a_position, 1.0);
v_fragPos     = worldPos.xyz;
v_normal      = normalize(normalMatrix * a_normal);

// Transformar tangente al world space manteniendo el handedness (w)
v_tangent     = vec4(normalize(normalMatrix * a_tangent.xyz), a_tangent.w);

v_texCoord    = a_texCoord;
gl_Position   = u_projection * u_view * worldPos;
