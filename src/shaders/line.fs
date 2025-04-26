#version 330

precision mediump float;

in float edge_dist;

out vec4 finalColor;

uniform vec3 color;

void main() {
  finalColor = vec4(color, 1.0-abs(edge_dist));
}
