#version 410 core

in vec2 v_texCoord;
in vec3 v_worldPos;

out vec4 fragColor;

void main() {
    vec2  tile    = floor(v_texCoord * 8.0);
    float checker = mod(tile.x + tile.y, 2.0);

    vec3 colorA = vec3(0.25, 0.45, 0.18);
    vec3 colorB = vec3(0.38, 0.62, 0.27);
    vec3 color  = mix(colorA, colorB, checker);

    fragColor = vec4(color, 1.0);
}
