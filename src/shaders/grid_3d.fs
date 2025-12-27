#version 330
precision highp float;

in vec3 v_color;
in float v_edge;
in vec3 v_pos;
in float v_normal_look_dir;

out vec4 out_color;

uniform vec3 bg_color;
uniform vec3 eye;

void main() {
  float eye_dist = length(eye - v_pos);
  vec3 bg = bg_color;
  float thick = exp2(-v_edge * v_edge * 1.0);
  thick *= v_normal_look_dir;
  float alpha = thick * pow(1.0 - eye_dist / 1000.0, 2.0);
  out_color = vec4(mix(bg, v_color, alpha), thick);
}
