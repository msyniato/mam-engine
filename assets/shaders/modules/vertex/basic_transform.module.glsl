// MODULE_INFO
// name: basic_transform
// stage: vertex
// priority: 100
// dependencies: []

// PARAMETERS
// uniform mat4 u_model      : null
// uniform mat4 u_view       : null
// uniform mat4 u_projection : null
// input   vec3 a_position
// input   vec3 a_normal
// input   vec4 a_tangent
// output  vec3 v_normal
// output  vec3 v_fragPos
// output  vec4 v_tangent

// FUNCTIONS

// MAIN_CODE
v_fragPos = vec3(u_model * vec4(a_position, 1.0));

mat3 normalMatrix = mat3(transpose(inverse(u_model)));
v_normal  = normalize(normalMatrix * a_normal);
v_tangent = vec4(normalize(normalMatrix * a_tangent.xyz), a_tangent.w);

gl_Position = u_projection * u_view * vec4(v_fragPos, 1.0);