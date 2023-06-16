#version 330
in vec4 vc;
in vec2 normal;

out vec4 finalColor;

uniform vec2 screen;
uniform vec4 resolution;
uniform vec2 zoom;

void main()
{
  float shade = 1 - smoothstep(length(normal), 0.0, .3) ;
  if (shade == 0) return;
  vec4 cshade = vec4(shade);
  vec4 r = resolution;
  r.x += 2;
  r.z -= 4;
  r.y += 2;
  r.w -= 4;
  finalColor = gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || 
   gl_FragCoord.y <= r.y || gl_FragCoord.y >= (r.y + r.w) ? vec4(0.) : (vc * clamp(cshade, 0., 1.));
}
