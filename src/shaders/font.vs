#version 330

precision mediump float;

in vec4 pos;
in vec4 color;
in vec4 clip_dists;
in float z;

out vec2 out_tpos;
out vec4 vs_color;

void main() {
  out_tpos = pos.zw;
  vs_color = color;

  gl_Position = vec4(pos.xy, z, 1.0);
  gl_ClipDistance[0] = clip_dists.x;
  gl_ClipDistance[1] = clip_dists.y;
  gl_ClipDistance[2] = clip_dists.z;
  gl_ClipDistance[3] = clip_dists.w;
}
