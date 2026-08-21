#version 450

// Vulkan material fragment template.
// IMPORTANT:
// We intentionally do NOT inject module uniforms here right now.
// Your current VKPipeline uniform buffer only maps u_model/u_view/u_projection.
// The samplers below match VKPipeline descriptor layout:
//   binding 0 = UBO
//   binding 1 = u_texture0
//   binding 2 = u_texture1
//   binding 3 = u_texture2
//   binding 4 = u_texture3

// INPUTS_INJECTION_POINT
// OUTPUTS_INJECTION_POINT

layout(set = 0, binding = 1) uniform sampler2D u_texture0;
layout(set = 0, binding = 2) uniform sampler2D u_texture1;
layout(set = 0, binding = 3) uniform sampler2D u_texture2;
layout(set = 0, binding = 4) uniform sampler2D u_texture3;

// FUNCTIONS_INJECTION_POINT

void main()
{
    // MAIN_CODE_INJECTION_POINT
}
