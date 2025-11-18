#version 330
precision mediump float;

in vec3 v_color;
out vec4 out_color;

void main(void) {
  out_color = vec4(v_color, 1.0);
}

