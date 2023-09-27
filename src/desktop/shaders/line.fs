#version 330
in vec2 normal;
in vec3 color;

out vec4 finalColor;

uniform vec2 screen;
uniform vec4 resolution;
uniform vec2 zoom;

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
  vec2 fragCoord = gl_FragCoord.xy - (resolution.xy * vec2(1, -1.));
  fragCoord.y -= screen.y - resolution.w;
  finalColor.rgba = (vec4(color, 1.0) * clamp(cshade, 0., 1.));
  finalColor.a = gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || fragCoord.y <= 2 || fragCoord.y + 2. >= resolution.w ? 0. : 1.;
}
