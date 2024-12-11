#version 330

precision mediump float;

in vec2 normal;
in vec3 color;

out vec4 finalColor;

void main()
{
  vec2 fragCoord = gl_FragCoord.xy;
  float shade = 1. - smoothstep(length(normal), 0.0, 0.3);
  if (shade == 0.) return;
  finalColor = vec4(color, 1.0) * clamp(shade, 0., 1.);
}
