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
  float shade = 1.f; //smoothstep(dist, 0, .03);
  r.xy += 2;
  r.zw -= 4;
  vec2 fragCoord = gl_FragCoord.xy - (resolution.xy * vec2(1, -1.));
  fragCoord.y -= screen.y - resolution.w;
  finalColor.rgb = color;
  finalColor.a = (gl_FragCoord.x <= r.x || gl_FragCoord.x >= (r.x + r.z) || fragCoord.y <= 2 || fragCoord.y + 2. >= resolution.w) ? 0. : shade;
}
