#version 330

#ifdef GL_EXT_clip_cull_distance
#  define BR_CULL
#endif

#ifdef GL_EXT_clip_cull_distance
#  define BR_CULL
#endif

precision highp float;

in vec4 vs_color;
in vec4 vs_clits;
in vec2 vs_dist_to_center;
in vec2 vs_size;

out vec4 out_color;

void main() {
#ifndef BR_CULL
  if (any(lessThan(vs_clits, vec4(0.0)))) discard;
#endif
  vec2 dist = abs(vs_dist_to_center);
  if (dist.x + dist.y + 5.0 > vs_size.x + vs_size.y) discard;
  //if (length(dist) + 1.0 > length(vs_size)) discard;

  out_color = vs_color;
  //out_color.r = abs(vs_dist_to_center.y) / 1000;
}
