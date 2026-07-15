#version 330 core

out vec4 FragColor;

in vec2 TexCoords;
in vec3 SkyPosition;

uniform sampler2D texture_diffuse1;
uniform float dayFactor; // 0.0 = noche, 1.0 = dia

void main()
{
    vec3 stars = texture(texture_diffuse1, TexCoords).rgb;

    // El modelo original no está centrado exactamente en (0, 0, 0).
    const vec3 skyCenter = vec3(-1.005, -6.601, 0.0);
    vec3 skyDirection = normalize(SkyPosition - skyCenter);
    float heightDirection = skyDirection.y;

    // -------------------------
    // CIELO NOCTURNO
    // -------------------------
    float horizonBlend = smoothstep(-0.04, 0.10, heightDirection);
    vec3 lowerNightSky = vec3(0.005, 0.008, 0.020);
    vec3 nightSky = mix(lowerNightSky, stars, horizonBlend);

    // -------------------------
    // CIELO DIURNO PROCEDURAL
    // -------------------------
    vec3 horizonColor = vec3(0.72, 0.86, 1.00);
    vec3 zenithColor = vec3(0.10, 0.42, 0.82);

    float skyGradient = smoothstep(-0.10, 0.85, heightDirection);
    vec3 daySky = mix(horizonColor, zenithColor, skyGradient);

    // Tono más claro cerca del horizonte.
    float horizonGlow = 1.0 - smoothstep(-0.05, 0.35, abs(heightDirection));
    daySky += vec3(0.12, 0.10, 0.07) * horizonGlow;

    // Sol procedural: no requiere otra textura ni otro modelo.
    vec3 sunDirection = normalize(vec3(-0.45, 0.55, -0.70));
    float sunAlignment = dot(skyDirection, sunDirection);
    float sunGlow = smoothstep(0.965, 0.9992, sunAlignment);
    float sunDisk = smoothstep(0.9992, 0.99982, sunAlignment);

    daySky += vec3(1.00, 0.72, 0.30) * sunGlow * 0.32;
    daySky += vec3(1.00, 0.96, 0.82) * sunDisk * 1.75;

    // Mezcla suave controlada desde C++ al pulsar N.
    vec3 finalSky = mix(nightSky, daySky, clamp(dayFactor, 0.0, 1.0));
    FragColor = vec4(finalSky, 1.0);
}
