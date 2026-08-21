// name: skybox_vert
// stage: vertex
// dependencies: []
// priority: 0
// PARAMETERS
// uniform mat4 u_view : null
// uniform mat4 u_projection : null
// input vec3 a_position
// output vec3 v_dir
// FUNCTIONS
// MAIN_CODE
v_dir = a_position;
mat4 viewNoTranslation = mat4(mat3(u_view));
vec4 clip = u_projection * viewNoTranslation * vec4(a_position, 1.0);
gl_Position = clip.xyww;