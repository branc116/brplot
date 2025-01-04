#version 330

precision mediump float;

in vec4 pos;
in float z;

out vec2 out_tpos;

void main() {
  out_tpos = pos.zw;
  gl_Position = vec4(pos.xy, z, 1.0);
}
