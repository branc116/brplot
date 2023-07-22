#version 330

in vec3 vertexPosition;
in vec3 vertexColor;
// This is not normal. This is dx, dy, and length of the line.
in vec3 vertexNormal;

out vec2 normal;
out vec3 color;
out float dist;

uniform vec2 offset;
uniform vec4 resolution;
uniform vec2 zoom;
uniform vec2 screen;

void main(void)
{
    dist = pow(1 - vertexPosition.z, 8.);
    normal = vertexNormal.xy;
    color = vertexColor;
    vec2 position = vertexPosition.xy;

    vec2 size = resolution.zw;

    //Don't know why, but this value works...
    float magic_number = size.y / screen.y * 2;

    vec2 fact = screen.xy / screen.yy;
    vec2 fact2 = resolution.ww / screen.xy;
    vec2 uv = position * magic_number;
    uv -= offset * fact2 * fact * 2;
    uv /= zoom * fact;
    uv += vec2(-1, 1.);
    uv += resolution.xy/screen.xy*vec2(2, -2);
    uv += resolution.zw/screen.xy*vec2(1, -1);
    gl_Position = vec4(uv, 0, 1.0);
}

