#version 330

in vec3 vertexPosition;
in vec4 vertexColor;

out vec4 vc;

uniform vec2 offset;
uniform vec2 resolution;
uniform vec2 zoom;

void main(void)
{
    vc = vertexColor;
    vec2 fact = resolution.xy / resolution.yy;
    vec2 uv = vertexPosition.xy - offset;
    uv /= fact * zoom;
    uv *= 2.0;
    gl_Position = vec4(uv, 0.0, 1.0);
}

