// name: normal_debug_vk
// stage: fragment
// dependencies: []
// priority: 0
// description: Vulkan normal debug fragment module.

// PARAMETERS
// input vec3 v_normal
// output vec4 outColor

// FUNCTIONS

// MAIN_CODE
outColor = vec4(normalize(v_normal) * 0.5 + 0.5, 1.0);
