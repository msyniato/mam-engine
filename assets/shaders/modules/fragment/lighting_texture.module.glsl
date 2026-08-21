// MODULE_INFO
// name: lighting_texture
// stage: fragment
// priority: 20
// dependencies: [lighting]
// description: Samples a diffuse texture

// PARAMETERS
// uniform sampler2D u_texture0
// input   vec2      v_texCoord
// input   vec3      v_normal
// input   vec3      v_fragPos

// FUNCTIONS

// MAIN_CODE
vec4 texColor = texture(u_texture0, v_texCoord);
vec3 lit      = calcLighting(texColor.rgb, normalize(v_normal), v_fragPos);
FragColor     = vec4(lit, texColor.a);
