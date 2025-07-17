#version 330
precision mediump float;

uniform sampler2D image;

in vec2 out_tpos;
out vec4 out_color;

void main() {
  vec4 col = texture(image, out_tpos);
  out_color = col;
  out_color.a = 1.0;
}

