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

uniform float spred;
uniform float px_round;
uniform float shadow_intesity;

float f1 = spred;
float fb = spred + px_round;

float round_factor() {
  vec2 dist = abs(vs_dist_to_center);
  float size_sum = vs_size.x + vs_size.y;
  float factor = smoothstep(size_sum, size_sum + f1, dist.x + dist.y + fb);
  return factor;
}

float shadow_factor() {
  vec2 dist = vs_dist_to_center;
  float size_sum = vs_size.x + vs_size.y;
  float factorx = smoothstep(vs_size.x, vs_size.x + f1, dist.x + fb);
  float factory = smoothstep(vs_size.y, vs_size.y + f1, dist.y + fb);
  return factorx + factory;
}

void main() {
#ifndef BR_CULL
  if (any(lessThan(vs_clits, vec4(0.0)))) discard;
#endif
  float rf = round_factor();
  if (rf >= 1.0) discard;
  //if (length(dist) + 1.0 > length(vs_size)) discard;

  float sf = shadow_factor();
  out_color = vs_color;
  out_color.rgb *= 1.0 + 0.1*shadow_intesity*sf;
  //out_color.r = abs(vs_dist_to_center.y) / 1000;
}
