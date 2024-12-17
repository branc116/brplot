#version 330

precision mediump float;

in vec2 out_tpos;
in vec4 outin_fg;
in vec4 outin_bg;
out vec4 out_color;
uniform sampler2D atlas;

void main() {
  float v0 = texture(atlas, out_tpos).r;
  out_color = mix(outin_bg, outin_fg, v0);
  //out_color = vec4(v0);
}
