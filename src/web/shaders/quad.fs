#version 100
precision mediump float;

varying vec2 normal;
varying vec3 color;
varying float dist;

uniform vec2 screen;
uniform vec4 resolution;
uniform vec2 zoom;

void main()
{
  vec4 r = resolution;
  r.x += 2.;
  r.z -= 4.;
  r.y += 2.;
  r.w -= 4.;
  gl_FragColor.rgba = vec4(color, 1.0);
  gl_FragColor.a *= gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || 
   gl_FragCoord.y <= r.y || gl_FragCoord.y >= (r.y + r.w) ? 0. : 1.;
}
