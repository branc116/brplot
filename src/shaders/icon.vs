#version 330
#ifdef GL_ANGLE_clip_cull_distance
#  extension GL_ANGLE_clip_cull_distance : enable
#endif

precision mediump float;

in vec4 pos;
in vec4 fg;
in vec4 bg;
in vec4 clip_dists;
in float z;

out vec2 out_tpos;
out vec4 outin_fg;
out vec4 outin_bg;

void main() {
  out_tpos = pos.zw;
  outin_fg = fg;
  outin_bg = bg;
  gl_Position = vec4(pos.xy, z, 1.0);
#ifdef GL_ANGLE_clip_cull_distance
  gl_ClipDistance[0] = clip_dists.x;
  gl_ClipDistance[1] = clip_dists.y;
  gl_ClipDistance[2] = clip_dists.z;
  gl_ClipDistance[3] = clip_dists.w;
#endif
}
