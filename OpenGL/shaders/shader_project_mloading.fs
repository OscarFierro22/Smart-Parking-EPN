#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

#define MAX_POINT_LIGHTS 24

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

uniform PointLight pointLights[MAX_POINT_LIGHTS];
uniform int activePointLights;

uniform vec3 viewPos;
uniform float dayFactor;

// =========================================================
// MATERIAL LEÍDO DESDE EL ARCHIVO MTL
// =========================================================

uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;
uniform vec3 materialEmissive;

uniform float materialShininess;
uniform float emissiveStrength;
uniform float materialOpacity;

uniform bool materialMetallic;

// =========================================================
// LUZ PUNTUAL
// =========================================================

vec3 calculatePointLight(
    PointLight light,
    vec3 normal,
    vec3 fragmentPosition,
    vec3 viewDirection
)
{
    vec3 lightDirection =
        normalize(
            light.position -
            fragmentPosition
        );

    // Iluminación ambiental.
    vec3 ambient =
        light.ambient *
        materialDiffuse;

    // Iluminación difusa.
    float diffuseIntensity =
        max(
            dot(
                normal,
                lightDirection
            ),
            0.0
        );

    vec3 diffuse =
        light.diffuse *
        diffuseIntensity *
        materialDiffuse;

    // =====================================================
    // ILUMINACIÓN ESPECULAR
    // =====================================================

    /*
        Usamos Blinn-Phong para obtener un reflejo
        más estable sobre la pintura del automóvil.
    */
    vec3 halfwayDirection =
        normalize(
            lightDirection +
            viewDirection
        );

    float safeShininess =
        clamp(
            materialShininess,
            1.0,
            1000.0
        );

    float specularIntensity =
        pow(
            max(
                dot(
                    normal,
                    halfwayDirection
                ),
                0.0
            ),
            safeShininess
        );

    vec3 finalSpecularColor =
        materialSpecular;

    float specularMultiplier =
        1.0;

    if (materialMetallic)
    {
        /*
            Los materiales metálicos reflejan parte
            del color propio de la superficie.
        */
        finalSpecularColor =
            mix(
                vec3(1.0),
                materialDiffuse,
                0.25
            );

        specularMultiplier =
            1.35;
    }

    vec3 specular =
        light.specular *
        specularIntensity *
        finalSpecularColor *
        specularMultiplier;

    // =====================================================
    // ATENUACIÓN
    // =====================================================

    float distanceValue =
        length(
            light.position -
            fragmentPosition
        );

    float attenuation =
        1.0 /
        (
            light.constant +
            light.linear *
            distanceValue +
            light.quadratic *
            distanceValue *
            distanceValue
        );

    return (
        ambient +
        diffuse +
        specular
    ) * attenuation;
}

// =========================================================
// LUZ DIURNA
// =========================================================

vec3 calculateDayLight(
    vec3 normal,
    vec3 viewDirection
)
{
    vec3 sunDirection =
        normalize(
            vec3(
                -0.45,
                0.80,
                -0.35
            )
        );

    vec3 sunColor =
        vec3(
            1.00,
            0.94,
            0.82
        );

    // Luz ambiental diurna.
    vec3 ambient =
        materialDiffuse *
        vec3(
            0.48,
            0.54,
            0.64
        );

    // Luz difusa diurna.
    float diffuseIntensity =
        max(
            dot(
                normal,
                sunDirection
            ),
            0.0
        );

    vec3 diffuse =
        materialDiffuse *
        sunColor *
        diffuseIntensity *
        0.95;

    // =====================================================
    // BRILLO DIURNO
    // =====================================================

    vec3 halfwayDirection =
        normalize(
            sunDirection +
            viewDirection
        );

    float safeShininess =
        clamp(
            materialShininess,
            1.0,
            1000.0
        );

    float specularIntensity =
        pow(
            max(
                dot(
                    normal,
                    halfwayDirection
                ),
                0.0
            ),
            safeShininess
        );

    vec3 finalSpecularColor =
        materialSpecular;

    float specularMultiplier =
        0.35;

    if (materialMetallic)
    {
        finalSpecularColor =
            mix(
                vec3(1.0),
                materialDiffuse,
                0.25
            );

        specularMultiplier =
            0.65;
    }

    vec3 specular =
        finalSpecularColor *
        sunColor *
        specularIntensity *
        specularMultiplier;

    return
        ambient +
        diffuse +
        specular;
}

// =========================================================
// PROGRAMA PRINCIPAL
// =========================================================

void main()
{
    vec3 normalizedNormal =
        normalize(Normal);

    vec3 viewDirection =
        normalize(
            viewPos -
            FragPos
        );

    // =====================================================
    // LUCES PUNTUALES (TECHO + ESTADOS)
    // =====================================================

    vec3 pointLightResult = vec3(0.0);
    for (
        int i = 0;
        i < activePointLights;
        ++i
    )
    {
        pointLightResult +=
            calculatePointLight(
                pointLights[i],
                normalizedNormal,
                FragPos,
                viewDirection
            );
    }

    // =====================================================
    // RESULTADO NOCTURNO
    // =====================================================

    vec3 nightResult =
        materialDiffuse *
        vec3(
            0.008,
            0.010,
            0.018
        ) +
        pointLightResult;

    // =====================================================
    // RESULTADO DIURNO
    // =====================================================

    // Los focos siguen existiendo durante el dia, aunque su contribucion se
    // reduce porque la iluminacion solar domina la escena.
    vec3 dayResult =
        calculateDayLight(
            normalizedNormal,
            viewDirection
        ) +
        pointLightResult * 0.22;

    // Transición entre noche y día.
    vec3 result =
        mix(
            nightResult,
            dayResult,
            clamp(
                dayFactor,
                0.0,
                1.0
            )
        );

    // El material emisivo hace visibles los tubos reales del OBJ de los
    // focos. No ilumina por sí solo; la iluminación del entorno proviene de
    // los point lights colocados en la misma posición física.
    result += materialEmissive * max(emissiveStrength, 0.0);

    // =====================================================
    // COLOR Y TRANSPARENCIA FINAL
    // =====================================================

    FragColor =
        vec4(
            result,
            clamp(
                materialOpacity,
                0.0,
                1.0
            )
        );
}