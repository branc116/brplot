#version 330

precision mediump float;

in float edge_dist;
in vec3 color;

out vec4 finalColor;

void main()
{
  finalColor = vec4(color, 1.0-abs(edge_dist));
}
