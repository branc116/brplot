#version 330

precision mediump float;

in vec4 pos;
in vec3 color;

out vec3 v_color;
out float v_edge;

uniform mat4 m_mvp;

void main() {
  v_color = color;
  v_edge = pos.w;
  gl_Position = m_mvp * vec4(pos.xyz, 1.0);
}
