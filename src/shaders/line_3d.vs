#version 330

precision mediump float;

in vec3 pos;
in vec3 normal;

out vec3 v_normal;
out vec3 v_pos;

uniform mat4 m_mvp;

void main(void) {
  v_normal = normal;
  v_pos = pos + normal;
  gl_Position = m_mvp * vec4(pos, 1.0);
}

