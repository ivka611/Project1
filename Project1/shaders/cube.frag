#version 330 core
out vec4 FragColor;

struct Material {
    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
    float ka; //ambient coefficient
    float kd; //diffuse coefficient
    float ks; //specular coefficient
    float shininess;
};

struct Light {
    vec3 position;
    vec3 diffuse; //the color of the light
};

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;
in vec2 EmissionCoords;

uniform vec3 viewPos;
uniform Material material;
uniform Light light;
uniform Light light2;
uniform sampler2D diffuseMap;
uniform sampler2D emissionMap;
uniform sampler2D specularMap;
uniform bool useSolidColor;
uniform vec3 solidColor;
uniform vec2 textureScale;
uniform bool unlitTexture;

void main()
{
    vec3 texColor = useSolidColor ? solidColor : texture(diffuseMap, TexCoords * textureScale).rgb;
    if (unlitTexture) {
        FragColor = vec4(texColor, 1.0);
        return;
    }

    vec3 specMap = useSolidColor ? vec3(0.0) : texture(specularMap, TexCoords).rgb;

    // ambient
    vec3 ambient = material.ka * texColor;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse * material.kd * (diff * texColor);

    // specular
    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = spec * specMap;

    // втора светлина
    vec3 lightDir2 = normalize(light2.position - FragPos);
    float diff2 = max(dot(norm, lightDir2), 0.0);
    vec3 diffuse2 = light2.diffuse * material.kd * (diff2 * texColor);

    vec3 reflectDir2 = reflect(-lightDir2, norm);
    float spec2 = pow(max(dot(viewDir, reflectDir2), 0.0), material.shininess);
    vec3 specular2 = spec2 * specMap;

    vec3 result = ambient + diffuse + specular + diffuse2 + specular2;
    FragColor = vec4(result, 1.0);
}