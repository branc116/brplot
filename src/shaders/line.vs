#version 330

precision mediump float;

in vec4 pos_delta;

out float edge_dist;

void main() {
  edge_dist = pos_delta.w;
  gl_Position = vec4(pos_delta.xyz, 1.0);
}
