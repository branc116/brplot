#version 330
precision mediump float;

uniform sampler2D image;
uniform vec2 mouse;
uniform vec2 size;

in vec2 out_tpos;
out vec4 out_color;

void main() {
  vec4 col = texture(image, out_tpos);
  out_color = col;

  vec2 mouse_uv = mouse / size;
  mouse_uv.y = 1.0 - mouse_uv.y;
  vec2 mouse_dist = mouse_uv - out_tpos;
  vec2 mid = 2.0 * mouse_uv - 1.0;
  float exponent = 10.0;
  float p = clamp(0.0, 1.0, pow(mid.x, exponent) + pow(mid.y, exponent));
  float edge_size = 0.008;
  vec2 moded = abs(vec2(out_tpos.x > 0.5 ? (out_tpos.x - 1.0) : out_tpos.x, out_tpos.y > 0.5 ? out_tpos.y - 1.0 : out_tpos.y));
  float factor = smoothstep(edge_size, 0.0, min(moded.x, moded.y));
  out_color.rgb *= 1.0 - factor;
  out_color.rgb += factor * 0.2 * (1.0 + 3.0 * (1.0 - length(mouse_dist)) * p * smoothstep(0.2*edge_size, 0.0, min(pow(mouse_dist.y, 2.0), pow(mouse_dist.x, 2.0))));
  out_color.a = 1.0;
}

