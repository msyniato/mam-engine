#version 430 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D gPos;
uniform sampler2D gNorm;
uniform sampler2D gAlbedoSpec;

// sizeof(Light) == 96 
struct Light{
    //LightType type;

    //16
    vec3 position;
    float padding0;

    //16
    vec3 ambient;
    float padding1;

    //16
    vec3 diffuse;
    float padding2;

    //16
    vec3 specular;
    float padding3;

    //16
    float constant_att;
    float linear_att;
    float quadratic_att;
    float pad4;

    //16
    float cut_off;
    float outer_cut_off;
    vec2 pad5;
};


layout(std430, binding = 2) buffer LightBuffer {
    Light lights[];
};

// Camera uniforms
uniform vec3 viewPos;
uniform int lightsNum;

vec3 calcLight(Light light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 albedo)
{
    vec3 lightDir = normalize(light.position - fragPos);
    float diff = max(dot(normal, lightDir), 0.0);

    float distance = length(light.position - fragPos);
    float attenuation = 1.0 / (light.constant_att + light.linear_att * distance + light.quadratic_att * (distance * distance));

    vec3 ambient  = light.ambient * albedo;
    vec3 diffuse  = light.diffuse * diff * albedo;

    ambient  *= attenuation;
    diffuse  *= attenuation;

    // TO-DO add specular 

    return ambient + diffuse;
}

void main()
{

    //vec3 Albedo = pow(texture(gAlbedoSpec, TexCoords).rgb, vec3(2.2)); // linearizing albedo due to texture format in defferred rendering pass.
    FragColor = vec4(texture(gAlbedoSpec, TexCoords).rgb, 1.0);
    //return;

    
    //if(lightsNum == 0){
    //  FragColor = vec4(Albedo, 1.0);
    //  return;
    //}

    //vec3 nTex = texture(gNorm, TexCoords).rgb;
    //// TO-DO make it less hardcoded
    //if (nTex == vec3(0.5)) {
    //    FragColor = vec4(Albedo, 1.0);
    //    return;
    //}

    //vec3 FragPos = texture(gPos, TexCoords).rgb;
    //vec3 Norm = normalize(nTex * 2.0 - 1.0);

    //vec3 lighting = vec3(0.0);
    //vec3 viewDir = normalize(viewPos - FragPos);

    //// TO-DO implement proper lighting techniques
    //for(int i = 0; i < lightsNum; i++)
    //    lighting += calcLight(lights[i], Norm, FragPos, viewDir, Albedo);
    
    //// Reinhard tone mapping
    //vec3 mapped = lighting / (lighting + vec3(1.0));

    //// Gamma correction
    //mapped = pow(mapped, vec3(1.0 / 2.2));

    //FragColor = vec4(Albedo, 1.0);
    
}