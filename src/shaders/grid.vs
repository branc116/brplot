#version 330

precision highp float;

in vec3 vert;

out float middle_dist;

void main() {
  gl_Position = vec4(vert.xy, 1.0, 1.0);
  middle_dist = vert.z;
}
