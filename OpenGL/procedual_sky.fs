#version 330 core

out vec4 FragColor;
in vec2 ScreenPosition;

uniform mat4 inverseProjection;
uniform mat4 inverseView;
uniform float dayFactor;
uniform float timeSeconds;

const float PI = 3.14159265359;

float hash21(vec2 p)
{
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);
    return fract(p.x * p.y);
}

void main()
{
    vec4 viewPosition = inverseProjection * vec4(ScreenPosition, 1.0, 1.0);
    vec3 viewDirection = normalize(viewPosition.xyz / max(viewPosition.w, 0.0001));
    vec3 direction = normalize(mat3(inverseView) * viewDirection);

    float height = clamp(direction.y, -1.0, 1.0);

    // Cielo nocturno completamente procedural: sin textura y sin modelo.
    vec3 nightHorizon = vec3(0.018, 0.030, 0.070);
    vec3 nightZenith = vec3(0.0015, 0.0030, 0.0120);
    float nightGradient = smoothstep(-0.10, 0.90, height);
    vec3 nightSky = mix(nightHorizon, nightZenith, nightGradient);

    vec2 sphericalUv = vec2(
        atan(direction.z, direction.x) / (2.0 * PI) + 0.5,
        asin(height) / PI + 0.5
    );

    vec2 starCell = floor(sphericalUv * vec2(1000.0, 500.0));
    float randomStar = hash21(starCell);
    float star = step(0.9968, randomStar);
    float starSize = smoothstep(0.9968, 1.0, randomStar);
    float twinkle = 0.80 + 0.20 * sin(timeSeconds * (1.2 + randomStar * 2.0) + randomStar * 40.0);
    float aboveHorizon = smoothstep(-0.02, 0.14, height);
    nightSky += vec3(0.75, 0.88, 1.0) * star * starSize * twinkle * aboveHorizon;

    // Cielo diurno procedural.
    vec3 dayHorizon = vec3(0.68, 0.84, 1.00);
    vec3 dayZenith = vec3(0.08, 0.36, 0.78);
    float dayGradient = smoothstep(-0.12, 0.92, height);
    vec3 daySky = mix(dayHorizon, dayZenith, dayGradient);

    vec3 sunDirection = normalize(vec3(-0.45, 0.62, -0.64));
    float sunAlignment = dot(direction, sunDirection);
    float sunGlow = smoothstep(0.955, 0.9990, sunAlignment);
    float sunDisk = smoothstep(0.9990, 0.99975, sunAlignment);
    daySky += vec3(1.00, 0.72, 0.30) * sunGlow * 0.30;
    daySky += vec3(1.00, 0.96, 0.83) * sunDisk * 1.60;

    float blendValue = smoothstep(0.0, 1.0, clamp(dayFactor, 0.0, 1.0));
    vec3 finalSky = mix(nightSky, daySky, blendValue);

    FragColor = vec4(finalSky, 1.0);
}
