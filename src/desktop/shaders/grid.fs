#version 330

precision mediump float;

//x, y, width, height
uniform vec4 resolution;
uniform vec2 zoom;
uniform vec2 offset;
uniform vec2 screen;

in vec2 fragTexCoord;
in vec2 glCoord;
out vec4 out_color;

vec2 log10(vec2 f) {
    return log(f) / log(10.0);
}

float map(vec2 cPos, vec2 zoom_level, vec2 offset) {
  vec2 fact = log10(zoom_level * 5.5);
  vec2 fr = fract(fact);
  vec2 baseMinor = floor(fact) - 1.0;
  vec2 baseMajor = baseMinor + 1;
  vec2 divs = pow(vec2(10), baseMinor);
  vec2 divsMajor = pow(vec2(10), baseMajor);
  cPos *= zoom_level;
  cPos += offset;
  vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
  vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;


  float thick = 1.3;
  vec2 to = divs/(80. * (1. + 10.0*(1. - fr)));
  vec2 d = 1. - smoothstep(abs(mcPosd), vec2(0.0), thick*to);
  vec2 dM = 1. - smoothstep(abs(mcPosdM), vec2(0.0), thick*to*4);
  float k = 20.;
  return max(max(d.x, d.y), max(dM.x, dM.y));
}

float map_outer(vec2 fragCoord) {
    return map((fragCoord )*0.5, zoom, offset);
}

void main(void) {
  vec2 glMax = abs(glCoord);
  float glM = glMax.x > glMax.y ? glMax.x : glMax.y;
  float res = map_outer(fragTexCoord);
  res += smoothstep(0.7, 1, fwidth(res));
  out_color = vec4(0.2, 0.3, 0.5, 1.0) * clamp(res, 0, 1);
  out_color += smoothstep(0.9, 1.0, pow(glM, 6.));
}
