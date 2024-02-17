#version 330
in vec2 normal;
in vec3 color;

out vec4 finalColor;

uniform vec2 screen;
uniform vec4 resolution;
uniform vec2 zoom;

void main()
{
  vec4 r = resolution;
  r.xy += 2;
  r.zw -= 4;
  vec2 fragCoord = gl_FragCoord.xy - (resolution.xy * vec2(1., -1.));
  fragCoord.y -= screen.y - resolution.w;
  if (gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || fragCoord.y <= 2 || fragCoord.y + 2. >= resolution.w) {
    finalColor = vec4(0.0);
    return;
  }

  float shade = 1. - smoothstep(length(normal), 0.0, .3);
  if (shade == 0.) return;
  finalColor = vec4(color, 1.0) * clamp(shade, 0., 1.);
}
