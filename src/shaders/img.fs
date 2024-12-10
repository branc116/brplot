#version 330
precision mediump float;

uniform sampler2D image;
in vec2 out_tpos;
out vec4 out_color;

void main() {
  vec3 col = texture(image, out_tpos).rgb;
  out_color = vec4(col, 1.0);
}

