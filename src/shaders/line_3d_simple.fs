#version 330

precision mediump float;

in vec3 normal;

out vec4 finalColor;

uniform vec3 color;

vec4 simple() {
  float fact = smoothstep(0., 1., 1. - length(normal));
  return vec4(color, 1.0) * fact;
}

void main() {
  finalColor = simple();
}
