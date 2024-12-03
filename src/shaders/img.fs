#version 330

precision mediump float;

uniform sampler2D image;

in vec2 in_tpos;

out vec4 out_color;

void main() {
  out_color = texture(image, in_tpos);
}
