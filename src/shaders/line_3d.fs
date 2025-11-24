#version 330

precision mediump float;

in vec3 v_normal;
in vec3 v_pos;

out vec4 finalColor;

uniform vec3 eye;
uniform vec3 color;

vec4 phong() {
  vec3 light = normalize(vec3(1., 1., 1.));
  vec3 norm_eye = normalize(eye);
  float ambient = 0.1;
  float direct = abs(dot(norm_eye, v_normal));
  float specular = dot(normalize(eye - v_pos), reflect(light, v_normal));
  specular = pow(clamp(specular, 0.0, 1.0), 8.0);
  return vec4((direct + ambient + specular) * color, pow(direct, 2.0));
}

void main() {
  finalColor = phong();
}
