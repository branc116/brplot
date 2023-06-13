#version 330
in vec4 vc;

out vec4 finalColor;

uniform vec2 screen;
uniform vec4 resolution;

void main()
{
  vec4 r = resolution;
  r.x += 2;
  r.z -= 4;
  r.y += 2;
  r.z -= 4;
  finalColor = gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || 
   gl_FragCoord.y <= r.y || gl_FragCoord.y >= (r.y + r.w) ? vec4(0.) : vc;
}
