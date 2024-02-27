#version 330
in vec3 pos;
in vec3 color;
in vec3 normal;

out vec4 finalColor;

uniform vec3 eye;
vec4 simple() {
  float fact = smoothstep(0, 1, 1 - length(normal));
  return vec4(color, 1.0) * fact;
}

vec4 phong() {
  vec3 light = normalize(vec3(1, 1, 1));
  vec3 norm_eye = normalize(eye);
  float direct = (dot(norm_eye, normal));
  float specular = pow(dot(reflect(normalize(pos - light), normal), norm_eye), 15.);
  return vec4((direct + specular * 1) * color, 1.0);
}

void main()
{
  finalColor = simple();
  //finalColor = phong();
}
