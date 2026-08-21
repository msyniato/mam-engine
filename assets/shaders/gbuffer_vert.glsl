#version 460
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec2 a_texCoord;
layout(location = 3) in vec4 a_tangent;

out vec3 v_fragPos;
out vec3 v_normal;
out vec2 v_texCoord;
out vec4 v_tangent;

uniform mat4 u_model, u_view, u_projection;

void main() {
    vec4 worldPos   = u_model * vec4(a_position, 1.0);
    v_fragPos       = worldPos.xyz;
    v_normal        = mat3(transpose(inverse(u_model))) * a_normal;
    v_texCoord      = a_texCoord;
    v_tangent       = a_tangent;
    gl_Position     = u_projection * u_view * worldPos;
}