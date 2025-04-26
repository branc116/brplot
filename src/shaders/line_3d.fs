#version 330

precision mediump float;

in vec3 normal;

out vec4 finalColor;

uniform vec3 eye;
uniform vec3 color;

vec4 phong() {
  vec3 light = normalize(vec3(1., 1., 1.));
  vec3 norm_eye = normalize(eye);
  float direct = abs(dot(norm_eye, normal));
  return vec4(direct * color, 1.0);
}

void main() {
  finalColor = phong();
}
