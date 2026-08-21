// MODULE_INFO
// name: terrain_splatting_ver
// stage: vertex
// dependencies: []
// priority: 6
// description: Passes world-space Y and normal to fragment for terrain splatting

// PARAMETERS
// uniform mat4 u_model : null
// input vec3 a_position
// input vec3 a_normal
// output float v_worldY
// output vec3 v_worldNormal
// output vec2 v_worldXZ

// FUNCTIONS

// MAIN_CODE
vec4 worldPos4 = u_model * vec4(a_position, 1.0);
v_worldY = worldPos4.y;
v_worldXZ     = worldPos4.xz;
v_worldNormal = normalize(mat3(transpose(inverse(u_model))) * a_normal);