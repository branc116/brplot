#version 330

precision mediump float;

in vec4 pos;
in vec4 fg;
in vec4 bg;
in float z;

out vec2 out_tpos;
out vec4 outin_fg;
out vec4 outin_bg;

void main() {
  out_tpos = pos.zw;
  outin_fg = fg;
  outin_bg = bg;
  gl_Position = vec4(pos.xy, z, 1.0);
}
