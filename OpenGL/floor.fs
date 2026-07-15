#version 330 core

out vec4 FragColor;
in vec3 WorldPosition;

uniform vec3 floorColor;

float randomValue(vec2 position)
{
    return fract(sin(dot(position, vec2(12.9898, 78.233))) * 43758.5453);
}

void main()
{
    // Variación muy leve para que el piso no se vea completamente plano.
    float grain = randomValue(floor(WorldPosition.xz * 0.75));
    vec3 asphalt = floorColor * mix(0.88, 1.08, grain);

    // Cuadrícula tenue de 10 unidades para dar escala al escenario.
    vec2 cell = abs(fract(WorldPosition.xz / 10.0) - 0.5);
    float edge = max(cell.x, cell.y);
    float grid = smoothstep(0.475, 0.495, edge);

    vec3 gridColor = vec3(0.085, 0.090, 0.095);
    vec3 finalColor = mix(asphalt, gridColor, grid * 0.28);

    FragColor = vec4(finalColor, 1.0);
}
