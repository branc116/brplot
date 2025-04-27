#version 330

precision mediump float;

uniform vec2 zoom;
uniform vec2 offset;
uniform vec2 screen;
uniform vec4 bg_color;
uniform vec4 lines_color;
uniform float line_thickness;
uniform float major_line_thickness;

in vec2 fragTexCoord;
out vec4 out_color;

vec2 log10(vec2 f) {
    return log(f) / log(10.0);
}

float log10f(float f) {
    return log(f) / log(10.0);
}

float map(vec2 cPos, vec2 zoom_level, vec2 offset) {
  float s = 3.6;
  vec2 fact = log10(zoom_level * s);
  vec2 fr = fract(fact);
  vec2 baseMinor = floor(fact) - 1.0;
  vec2 baseMajor = baseMinor + 1.;
  vec2 divs = pow(vec2(10.), baseMinor);
  vec2 divsMajor = pow(vec2(10.), baseMajor);
  cPos *= zoom_level;
  cPos += offset;
  vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
  vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;
  vec2 thick = line_thickness * 0.01 / s / (1.0 + pow(screen, vec2(0.5, 0.5)));
  vec2 to = divs / (1.0 + (1.0 - log10f(s))-fr);
  vec2 d = 1. - smoothstep(abs(mcPosd), vec2(-0.000), thick*to);
  vec2 dM = 1. - smoothstep(abs(mcPosdM), vec2(-0.000), thick*to*major_line_thickness);
  //return (d.x) + (d.y) + (dM.x) + (dM.y);
  return max(max(d.x, d.y), max(dM.x, dM.y));
}

float map_outer(vec2 fragCoord) {
    return map((fragCoord)*0.5, zoom, offset);
}

void main(void) {
  float aa = 0.;
  vec2 d = vec2(dFdx(fragTexCoord.x), dFdy(fragTexCoord.y)) * 1.;
  float c = aa*2.+1.;
  float res = 0.;
  for( float x = -aa ; x <= aa ; ++x)
    for( float y = -aa ; y <= aa ; ++y)
      res += map_outer(fragTexCoord+vec2(x*d.x, y*d.y)/(aa+1.))/(c*c);

  float s = smoothstep(-0.0002, 0.001, res);
  out_color = mix(bg_color, lines_color, s > 0.4 ? s : 0.0);
}
