// name: basic_texture_vk
// stage: fragment
// dependencies: []
// priority: 0
// description: Vulkan texture fragment module using u_texture0.

// PARAMETERS
// uniform sampler2D u_texture0
// input vec2 v_texCoord
// output vec4 outColor

// FUNCTIONS

// MAIN_CODE
outColor = texture(u_texture0, v_texCoord);
