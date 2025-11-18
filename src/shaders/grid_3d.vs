#version 330

precision mediump float;

in vec3 pos;
in vec3 color;

out vec3 v_color;

uniform mat4 m_mvp;

void main() {
  v_color = color;
  gl_Position = m_mvp * vec4(pos.xyz, 1.0);
}
