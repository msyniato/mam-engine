#version 460

layout(location = 0) in vec3 a_position;

#ifdef MAM_VULKAN

layout(set = 0, binding = 0) uniform CameraUBO {
    mat4 u_view;
    mat4 u_model;
    mat4 u_projection;
    vec3 u_viewPos;
    int u_lightCount;
    vec3 u_baseColor;
    float u_metallic;
    float u_roughness;
    int u_useAlbedoMap;
    int u_useNormalMap;
    int u_useRoughnessMap;
    int u_useMetallicMap;
} ubo;

void main() {
    gl_Position = ubo.u_projection * ubo.u_model * vec4(a_position, 1.0);
}

#else

uniform mat4 u_lightSpace;
uniform mat4 u_model;

void main() {
    gl_Position = u_lightSpace * u_model * vec4(a_position, 1.0);
}

#endif