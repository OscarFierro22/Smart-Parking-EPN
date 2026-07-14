#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

#define NR_POINT_LIGHTS 108

struct PointLight
{
    vec3 position;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;

    float constant;
    float linear;
    float quadratic;
};

uniform PointLight pointLights[NR_POINT_LIGHTS];
uniform vec3 viewPos;

uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

vec3 CalculatePointLight(
    PointLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
);

void main()
{
    vec3 normalizedNormal = normalize(Normal);
    vec3 viewDirection = normalize(viewPos - FragPos);

    vec3 result = vec3(0.0f);

    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        result += CalculatePointLight(
            pointLights[i],
            normalizedNormal,
            FragPos,
            viewDirection
        );
    }

    FragColor = vec4(result, 1.0f);
}

vec3 CalculatePointLight(
    PointLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
)
{
    vec3 lightDirection =
        normalize(light.position - fragmentPosition);

    // Componente ambiental
    vec3 ambient =
        light.ambient * materialDiffuse;

    // Componente difusa
    float diffuseIntensity =
        max(dot(normal, lightDirection), 0.0f);

    vec3 diffuse =
        light.diffuse *
        diffuseIntensity *
        materialDiffuse;

    // Componente especular
    vec3 reflectionDirection =
        reflect(-lightDirection, normal);

    float specularIntensity = pow(
        max(dot(viewDirection, reflectionDirection), 0.0f),
        materialShininess
    );

    vec3 specular =
        light.specular *
        specularIntensity *
        materialSpecular;

    // Distancia y atenuación
    float distance =
        length(light.position - fragmentPosition);

    float attenuation =
        1.0f /
        (
            light.constant +
            light.linear * distance +
            light.quadratic * distance * distance
        );

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;

    return ambient + diffuse + specular;
}