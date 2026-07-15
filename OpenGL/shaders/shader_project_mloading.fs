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
uniform float dayFactor; // 0.0 = noche, 1.0 = dia

uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

vec3 CalculatePointLight(
    PointLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
);

vec3 CalculateDayLight(
    vec3 normal,
    vec3 viewDirection
);

void main()
{
    vec3 normalizedNormal = normalize(Normal);
    vec3 viewDirection = normalize(viewPos - FragPos);

    // Durante la noche se conservan las 108 luces del parqueadero.
    vec3 nightResult = vec3(0.0);

    for (int i = 0; i < NR_POINT_LIGHTS; i++)
    {
        nightResult += CalculatePointLight(
            pointLights[i],
            normalizedNormal,
            FragPos,
            viewDirection
        );
    }

    // Durante el día se usa iluminación ambiental y direccional del sol.
    vec3 dayResult = CalculateDayLight(
        normalizedNormal,
        viewDirection
    );

    vec3 result = mix(
        nightResult,
        dayResult,
        clamp(dayFactor, 0.0, 1.0)
    );

    FragColor = vec4(result, 1.0);
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

    vec3 ambient =
        light.ambient * materialDiffuse;

    float diffuseIntensity =
        max(dot(normal, lightDirection), 0.0);

    vec3 diffuse =
        light.diffuse *
        diffuseIntensity *
        materialDiffuse;

    vec3 reflectionDirection =
        reflect(-lightDirection, normal);

    float specularIntensity = pow(
        max(dot(viewDirection, reflectionDirection), 0.0),
        max(materialShininess, 1.0)
    );

    vec3 specular =
        light.specular *
        specularIntensity *
        materialSpecular;

    float distance =
        length(light.position - fragmentPosition);

    float attenuation =
        1.0 /
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

vec3 CalculateDayLight(
    vec3 normal,
    vec3 viewDirection
)
{
    // Dirección desde el fragmento hacia el sol.
    vec3 sunDirection = normalize(vec3(-0.45, 0.80, -0.35));
    vec3 sunColor = vec3(1.00, 0.94, 0.82);

    vec3 ambient =
        materialDiffuse * vec3(0.48, 0.54, 0.64);

    float diffuseIntensity =
        max(dot(normal, sunDirection), 0.0);

    vec3 diffuse =
        materialDiffuse * sunColor * diffuseIntensity * 0.95;

    vec3 reflectionDirection =
        reflect(-sunDirection, normal);

    float specularIntensity = pow(
        max(dot(viewDirection, reflectionDirection), 0.0),
        max(materialShininess, 1.0)
    );

    vec3 specular =
        materialSpecular * sunColor * specularIntensity * 0.35;

    return ambient + diffuse + specular;
}
