// name: basic_transform_vk
// stage: vertex
// dependencies: []
// priority: 0
// description: Vulkan basic transform module using model/view/projection matrices.

// PARAMETERS
// uniform mat4 u_model
// uniform mat4 u_view
// uniform mat4 u_projection
// input vec3 a_position
// input vec3 a_normal
// input vec2 a_texCoord
// output vec3 v_fragPos
// output vec3 v_normal
// output vec2 v_texCoord

// FUNCTIONS

// MAIN_CODE
vec4 worldPos = u_model * vec4(a_position, 1.0);
v_fragPos = worldPos.xyz;
v_normal = mat3(transpose(inverse(u_model))) * a_normal;
v_texCoord = a_texCoord;
gl_Position = u_projection * u_view * worldPos;
