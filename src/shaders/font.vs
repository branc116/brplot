#version 330

precision mediump float;

in vec4 pos;
in vec4 color;
in float z;

out vec2 out_tpos;
out vec4 vs_color;

void main() {
  out_tpos = pos.zw;
  vs_color = color;

  gl_Position = vec4(pos.xy, z, 1.0);
}
