#version 330

in vec3 vertexPosition;
in vec4 vertexColor;

out vec4 vc;

uniform vec2 offset;
uniform vec4 resolution;
uniform vec2 zoom;
uniform vec2 screen;

void main(void)
{
    vc = vertexColor;
    vec2 fact = resolution.zw / resolution.ww;
    vec2 uv = vertexPosition.xy - offset;
    uv /= fact * zoom;
    uv *= 2.0;
    uv *= resolution.zw/screen.xy;
    uv += (resolution.xy / screen.xy) / fact;
    gl_Position = vec4(uv, 0.0, 1.0);
}

