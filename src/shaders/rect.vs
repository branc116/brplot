#version 330

#ifdef GL_EXT_clip_cull_distance
#  extension GL_ANGLE_clip_cull_distance : enable
#  define BR_CULL
#endif

#ifdef GL_EXT_clip_cull_distance
#  extension GL_EXT_clip_cull_distance : enable
#  define BR_CULL
#endif

precision highp float;

in vec4 pos;
in vec4 color;
in vec4 clip_dists;
in vec2 z_and_round_factor;

uniform vec4 viewport; // x,y, width, height

out vec4 vs_color;
out vec4 vs_clits;
out vec2 vs_dist_to_center;
out vec2 vs_size;

vec2 to_ndc(vec2 point, vec2 viewport_size) {
  point.y = viewport_size.y - point.y;
  return point / viewport_size * 2.0 - 1.0;
}

void main() {
  vs_color = color;
  vs_clits = clip_dists;
  vs_dist_to_center = pos.xy - pos.zw;
  vs_size = abs(vs_dist_to_center);

  gl_Position = vec4(to_ndc(pos.xy, viewport.zw), z_and_round_factor.x, 1.0);
#ifdef BR_CULL
  gl_ClipDistance[0] = clip_dists.x;
  gl_ClipDistance[1] = clip_dists.y;
  gl_ClipDistance[2] = clip_dists.z;
  gl_ClipDistance[3] = clip_dists.w;
#endif
}
