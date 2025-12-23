#version 330
precision mediump float;

in vec4 pos;
in vec3 normal;
in vec3 color;

out vec3 v_color;
out float v_edge;
out vec3 v_pos;
out float v_normal_look_dir;

uniform mat4 m_mvp;
uniform vec3 eye;
uniform vec3 target;

void main() {
  v_normal_look_dir = abs(dot(normalize(target - eye), normal));
  v_normal_look_dir = pow(v_normal_look_dir, 2.0);
  if (v_normal_look_dir < 0.3) {
    gl_Position = vec4(10.0, 10.0, 10.0, 1.0);
    return;
  }
  v_color = color;
  v_edge = pos.w;
  gl_Position = m_mvp * vec4(pos.xyz, 1.0);
  v_pos = pos.xyz;
}
