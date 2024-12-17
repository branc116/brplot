#version 330

precision mediump float;

in vec2 out_tpos;
in vec4 vs_color;
in vec2 inout_pos;
out vec4 out_color;

uniform vec2 mouse;
uniform sampler2D atlas;
uniform vec3 sub_pix_aa_map;
uniform float sub_pix_aa_scale;

void main() {
  vec4 v0 = texture(atlas, out_tpos);
  float sum = v0.x;
  float dx = dFdx(sum);
  vec3 c = sub_pix_aa_map * dx * sub_pix_aa_scale + sum;
  float length = clamp(length(inout_pos - mouse) / 100.0, 0.0, 1.0);

  out_color = vs_color * vec4(c, 1.);
}
