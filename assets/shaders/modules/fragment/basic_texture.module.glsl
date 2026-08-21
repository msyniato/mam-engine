// MODULE_INFO
// name: basic_texture
// stage: fragment
// dependencies: []
// priority: 10
// description: Samples a diffuse texture

// PARAMETERS
// uniform sampler2D u_texture0
// input vec2 v_texCoord

// FUNCTIONS

// MAIN_CODE
FragColor = texture(u_texture0, v_texCoord);