#version 330
in vec2 normal;

out vec4 finalColor;

uniform vec2 screen;
uniform vec4 resolution;
uniform vec2 zoom;
uniform vec3 color;

void main()
{
  float shade = 1. - smoothstep(length(normal), 0.0, .3) ;
  if (shade == 0.) return;
  vec4 cshade = vec4(shade);
  vec4 r = resolution;
  r.x += 2;
  r.z -= 4;
  r.y += 2;
  r.w -= 4;
  finalColor.rgba = (vec4(color, 1.0) * clamp(cshade, 0., 1.));
  finalColor.a = gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || 
   gl_FragCoord.y <= r.y || gl_FragCoord.y >= (r.y + r.w) ? 0. : 1.;
}
