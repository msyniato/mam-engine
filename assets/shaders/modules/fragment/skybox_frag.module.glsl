// name: skybox_frag
// stage: fragment
// dependencies: []
// priority: 0
// PARAMETERS
// uniform samplerCube u_texture0 : null
// input vec3 v_dir
// FUNCTIONS
// MAIN_CODE
FragColor = texture(u_texture0, v_dir);