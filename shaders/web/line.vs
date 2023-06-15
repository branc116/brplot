#version 100
precision mediump float;

attribute vec3 vertexPosition;
// This is not normal. This is dx, dy, and length of the line.
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

varying vec4 vc;
varying vec2 normal;

uniform vec2 offset;
uniform vec4 resolution;
uniform vec2 zoom;
uniform vec2 screen;

void main(void)
{
    vc = vec4(normalize(vertexColor));
    normal = vec2(0);
    normal = -vertexPosition.z * normalize(vertexNormal.yx * zoom * vec2(-1, 1));
    vec2 position = vertexPosition.xy;
      position += normal * 0.05 * (1. - min(zoom.xy*.1, 20000.0));
    vec2 size = resolution.zw;

    //Don't know why, but this value works...
    float magic_number = size.y / screen.y * 2.;

    vec2 fact = screen.xy / screen.yy;
    vec2 fact2 = resolution.ww / screen.xy;
    vec2 uv = position * magic_number;
    uv -= offset * fact2 * fact * 2.;
    uv /= zoom * fact;
    uv += vec2(-1., 1.);
    uv += resolution.xy/screen.xy*vec2(2., -2.);
    uv += resolution.zw/screen.xy*vec2(1., -1.);
    gl_Position = vec4(uv, 0., 1.0);
}

