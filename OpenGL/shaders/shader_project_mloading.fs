#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

uniform vec3 lightPos;
uniform vec3 lightColor;
uniform vec3 viewPos;

uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform float materialShininess;

void main()
{
    float ambientStrength = 0.45;
    vec3 ambient = ambientStrength * lightColor * materialDiffuse;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor * materialDiffuse;

    vec3 viewDir = normalize(viewPos - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(
        max(dot(viewDir, reflectDir), 0.0),
        materialShininess
    );

    vec3 specular =
        spec * lightColor * materialSpecular;

    vec3 result = ambient + diffuse + specular;

    FragColor = vec4(result, 1.0);
}