#version 330 core

layout (location = 0) in vec2 aPosition;
layout (location = 1) in vec4 aColor;

out vec4 VertexColor;
uniform vec2 screenSize;

void main()
{
    float x = (aPosition.x / screenSize.x) * 2.0 - 1.0;
    float y = 1.0 - (aPosition.y / screenSize.y) * 2.0;
    gl_Position = vec4(x, y, 0.0, 1.0);
    VertexColor = aColor;
}
