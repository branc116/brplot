#version 330
in vec2 normal;
in vec3 color;
in float dist;

out vec4 finalColor;

uniform vec2 screen;
uniform vec4 resolution;
uniform vec2 zoom;

void main()
{
  vec4 r = resolution;
  r.x += 2;
  r.z -= 4;
  r.y += 2;
  r.w -= 4;
  finalColor.rgba = pow((1 - dist*dist), 16.) * vec4(color, 1.0);
  finalColor.a *= gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || 
   gl_FragCoord.y <= r.y || gl_FragCoord.y >= (r.y + r.w) ? 0. : 1.;
}
