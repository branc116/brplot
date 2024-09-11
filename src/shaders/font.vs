#version 330

precision mediump float;

in vec4 pos;

out vec2 out_tpos;

uniform vec2 resolution;

void main() {
  vec2 norm = (pos.xy) / resolution;
  norm *= vec2(2.0, -2.0);
  norm += vec2(-1.0, 1.0);

  out_tpos = pos.zw;

  gl_Position = vec4(norm, 0.0, 1.0);
}
