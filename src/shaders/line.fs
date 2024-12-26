#version 330

precision mediump float;

in float edge_dist;
in vec3 color;

out vec4 finalColor;

void main()
{
  float shade = smoothstep(1.0, 0.0, abs(edge_dist));
  finalColor = vec4(color*shade, 1.0-abs(edge_dist));
}
