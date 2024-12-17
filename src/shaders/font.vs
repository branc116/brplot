#version 330

precision mediump float;

in vec4 pos;
in vec4 color;

out vec2 out_tpos;
out vec4 vs_color;
out vec2 inout_pos;

uniform vec2 resolution;

void main() {
  inout_pos = pos.xy;
  vec2 norm = (pos.xy) / resolution;
  norm *= vec2(2.0, -2.0);
  norm += vec2(-1.0, 1.0);

  out_tpos = pos.zw;
  vs_color = color;

  gl_Position = vec4(norm, -0.1, 1.0);
}
