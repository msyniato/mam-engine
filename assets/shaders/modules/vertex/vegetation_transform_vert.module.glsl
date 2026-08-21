// MODULE_INFO
// name: vegetation_transform_vert
// stage: vertex
// dependencies: []
// priority: 5

// PARAMETERS
// uniform mat4 u_view
// uniform mat4 u_projection
// input vec3 a_position
// input vec3 a_normal
// input vec2 a_texCoord
// input mat4 a_instanceMatrix
// output vec3 v_normal
// output vec2 v_texCoord
// output vec3 v_worldPos

// FUNCTIONS

// MAIN_CODE
vec4 worldPos4    = a_instanceMatrix * vec4(a_position, 1.0);
gl_Position       = u_projection * u_view * worldPos4;
v_worldPos        = worldPos4.xyz;
v_texCoord        = a_texCoord;
mat3 normalMat    = transpose(inverse(mat3(a_instanceMatrix)));
v_normal          = normalize(normalMat * a_normal);