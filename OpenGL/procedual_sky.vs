#version 330 core

out vec2 ScreenPosition;

void main()
{
    const vec2 vertices[3] = vec2[](
        vec2(-1.0, -1.0),
        vec2( 3.0, -1.0),
        vec2(-1.0,  3.0)
    );

    ScreenPosition = vertices[gl_VertexID];
    gl_Position = vec4(ScreenPosition, 0.0, 1.0);
}
