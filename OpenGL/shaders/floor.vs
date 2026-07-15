#version 330 core

layout (location = 0) in vec3 aPos;

out vec3 WorldPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 worldPosition = model * vec4(aPos, 1.0);
    WorldPosition = worldPosition.xyz;
    gl_Position = projection * view * worldPosition;
}
