#version 330

#ifdef BR_CULL
#  extension GL_ANGLE_clip_cull_distance : enable
#endif

precision mediump float;

in vec4 pos;
in vec4 fg;
in vec4 bg;
in vec4 clip_dists;
in float z;

out vec2 vs_tpos;
out vec4 vs_fg;
out vec4 vs_bg;

void main() {
  vs_tpos = pos.zw;
  vs_fg = fg;
  vs_bg = bg;

  gl_Position = vec4(pos.xy, z, 1.0);
#ifdef BR_CULL
  gl_ClipDistance[0] = clip_dists.x;
  gl_ClipDistance[1] = clip_dists.y;
  gl_ClipDistance[2] = clip_dists.z;
  gl_ClipDistance[3] = clip_dists.w;
#endif
}
