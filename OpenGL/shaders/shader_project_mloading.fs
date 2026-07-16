#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoords;

// =========================================================
// CONFIGURACIÓN DE LUCES
// =========================================================

/*
    Se conserva la capacidad del repositorio para manejar
    las 108 luces del parqueadero.
*/
#define MAX_POINT_LIGHTS 108

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

/*
    Permite indicar desde C++ cuántas luces están activas.

    Si el código todavía no envía este uniform, su valor
    será cero y el shader utilizará las 108 luces para
    mantener compatibilidad con el repositorio.
*/
uniform int activePointLights;

uniform vec3 viewPos;

/*
    0.0 = noche.
    1.0 = día.
*/
uniform float dayFactor;

// =========================================================
// MATERIAL LEÍDO DESDE EL ARCHIVO MTL
// =========================================================

uniform vec3 materialDiffuse;
uniform vec3 materialSpecular;

uniform float materialShininess;
uniform float materialOpacity;

uniform bool materialMetallic;

// =========================================================
// CALCULAR UNA LUZ PUNTUAL
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

    // -----------------------------------------------------
    // Componente ambiental
    // -----------------------------------------------------

    vec3 ambient =
        light.ambient *
        materialDiffuse;

    // -----------------------------------------------------
    // Componente difusa
    // -----------------------------------------------------

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

    // -----------------------------------------------------
    // Componente especular mediante Blinn-Phong
    // -----------------------------------------------------

    /*
        Blinn-Phong genera un brillo más estable para la
        pintura y los materiales metálicos del vehículo.
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
            En superficies metálicas, una parte del reflejo
            adopta el color propio del material.
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

    // -----------------------------------------------------
    // Atenuación por distancia
    // -----------------------------------------------------

    float distanceValue =
        length(
            light.position -
            fragmentPosition
        );

    float attenuationDenominator =
        light.constant +
        light.linear *
        distanceValue +
        light.quadratic *
        distanceValue *
        distanceValue;

    /*
        Se evita una división por cero en caso de que una
        luz tenga parámetros de atenuación incorrectos.
    */
    float attenuation =
        1.0 /
        max(
            attenuationDenominator,
            0.0001
        );

    return
        (
            ambient +
            diffuse +
            specular
        ) *
        attenuation;
}

// =========================================================
// CALCULAR ILUMINACIÓN DIURNA
// =========================================================

vec3 calculateDayLight(
    vec3 normal,
    vec3 viewDirection
)
{
    /*
        Dirección desde el fragmento hacia el sol.
    */
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

    // -----------------------------------------------------
    // Luz ambiental diurna
    // -----------------------------------------------------

    vec3 ambient =
        materialDiffuse *
        vec3(
            0.48,
            0.54,
            0.64
        );

    // -----------------------------------------------------
    // Luz difusa diurna
    // -----------------------------------------------------

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

    // -----------------------------------------------------
    // Luz especular diurna
    // -----------------------------------------------------

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
        normalize(
            Normal
        );

    vec3 viewDirection =
        normalize(
            viewPos -
            FragPos
        );

    // =====================================================
    // DETERMINAR EL NÚMERO DE LUCES
    // =====================================================

    int lightCount =
        activePointLights;

    /*
        Compatibilidad con el código actual del repositorio:

        si activePointLights todavía no se configura desde
        C++, su valor será cero y se utilizarán las 108 luces.
    */
    if (
        lightCount <= 0
    )
    {
        lightCount =
            MAX_POINT_LIGHTS;
    }

    lightCount =
        clamp(
            lightCount,
            0,
            MAX_POINT_LIGHTS
        );

    // =====================================================
    // RESULTADO NOCTURNO
    // =====================================================

    /*
        Se agrega una iluminación ambiental mínima para
        evitar que las superficies sin una luz cercana se
        vuelvan completamente negras.
    */
    vec3 nightResult =
        materialDiffuse *
        vec3(
            0.008,
            0.010,
            0.018
        );

    for (
        int i = 0;
        i < MAX_POINT_LIGHTS;
        ++i
    )
    {
        /*
            En GLSL resulta más compatible mantener un límite
            constante y detener el ciclo según lightCount.
        */
        if (
            i >= lightCount
        )
        {
            break;
        }

        nightResult +=
            calculatePointLight(
                pointLights[i],
                normalizedNormal,
                FragPos,
                viewDirection
            );
    }

    // =====================================================
    // RESULTADO DIURNO
    // =====================================================

    vec3 dayResult =
        calculateDayLight(
            normalizedNormal,
            viewDirection
        );

    // =====================================================
    // TRANSICIÓN NOCHE-DÍA
    // =====================================================

    float safeDayFactor =
        clamp(
            dayFactor,
            0.0,
            1.0
        );

    vec3 result =
        mix(
            nightResult,
            dayResult,
            safeDayFactor
        );

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