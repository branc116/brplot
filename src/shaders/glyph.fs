#version 330

#ifdef GL_EXT_clip_cull_distance
#  define BR_CULL
#endif

#ifdef GL_EXT_clip_cull_distance
#  define BR_CULL
#endif

precision highp float;

in vec2 vs_tpos;
in vec4 vs_fg;
in vec4 vs_bg;
in vec4 vs_clits;
out vec4 out_color;

uniform sampler2D atlas;

void main() {
#ifndef BR_CULL
  if (any(lessThan(vs_clits, vec4(0.0)))) discard;
#endif

  float v0 = texture(atlas, vs_tpos).x;
  float sum = pow(v0, 0.5);
  out_color = mix(vs_bg, vs_fg, sum);
  if (out_color.a < 0.4) discard;
}
