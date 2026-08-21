#version 460

in  vec2 v_texCoord;
out vec4 FragColor;

uniform sampler2D u_ssaoInput; // raw SSAO result from the first pass

void main()
{
    // 4x4 box blur — matches the 4x4 noise tile size so one full noise period
    // is averaged out, effectively removing the visible noise pattern while
    // keeping the low-frequency occlusion signal intact.
    vec2 texelSize = 1.0 / vec2(textureSize(u_ssaoInput, 0));

    float result = 0.0;
    for (int x = -2; x < 2; ++x)
    for (int y = -2; y < 2; ++y)
    {
        vec2 offset = vec2(float(x), float(y)) * texelSize;
        result += texture(u_ssaoInput, v_texCoord + offset).r;
    }

    float ao = result / 16.0; // 4x4 = 16 samples
    FragColor = vec4(ao, ao, ao, 1.0);
}
