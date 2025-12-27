#version 330

precision highp float;

uniform vec4 bg_color;
uniform vec4 lines_color;

in float middle_dist;
out vec4 out_color;

float br_ramp(float thick, float value) {
  float factor =  smoothstep(-thick, 0.0, value)
                 -smoothstep(0.0, thick, value);
  return factor;
}

void main(void) {
  float factor =  br_ramp(0.4, middle_dist*2.0);
  out_color = mix(bg_color, lines_color, factor);
  out_color.a = mix(0.0, 1.0, factor);
}
