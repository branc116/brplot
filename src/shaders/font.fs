#version 330

precision mediump float;

in vec2 vs_tpos;
in vec4 vs_fg;
in vec4 vs_bg;
out vec4 out_color;

uniform sampler2D atlas;

void main() {
  vec4 v0 = texture(atlas, vs_tpos);
  float sum = pow(v0.x, 0.5);

  out_color = mix(vs_bg, vs_fg, sum);
}
