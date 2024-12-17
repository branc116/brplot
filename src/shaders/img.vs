#version 330

precision mediump float;

in vec4 pos;
uniform vec2 resolution;

out vec2 out_tpos;

void main() {
  vec2 norm = (pos.xy) / resolution;
  norm *= vec2(2.0, -2.0);
  norm += vec2(-1.0, 1.0);

  out_tpos = vec2(pos.z, 1.0 - pos.w);
  gl_Position = vec4(norm, -0.2, 1.0);
}
