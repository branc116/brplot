#version 330

precision mediump float;

//x, y, width, height
uniform vec2 resolution;
uniform vec3 eye;
uniform vec3 look_dir;

in vec3 fragTexCoord;
in vec3 normal;

out vec4 out_color;

vec2 log10(vec2 f) {
    return log(f) / log(10.0);
}

float map(vec2 cPos, vec2 zoom_level) {
    vec2 fact = log10(zoom_level * 4.5);
    vec2 fr = fract(fact);
    vec2 baseMinor = floor(fact) - 1.0;
    vec2 baseMajor = baseMinor + 1.;
    vec2 divs = pow(vec2(10.), baseMinor);
    vec2 divsMajor = pow(vec2(10.), baseMajor);
    vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
    vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;

    vec2 to = 0.5 * divs/(80. * (1. + 10.*(1. - 1.*fr)));
    vec2 f = normal.z > normal.y ?
       1.0 + abs(cPos - eye.xy) / abs(eye.z) :
       1.0 + abs(cPos - eye.xz) / abs(eye.y);
    
    vec2 d = 1. - smoothstep(abs(mcPosd), vec2(0.0), to * f);
    vec2 dM = 1. - smoothstep(abs(mcPosdM), vec2(0.0), 5.0 * to * f);
    float k = 20.;

    return max(max(d.x, d.y), max(dM.x, dM.y));
}

float map_outer(vec2 fragCoord) {
  float scale = length(eye - fragTexCoord);
  float scale2 = (1.0 - abs(dot(normal, normalize(look_dir))));
  //scale *= scale2;
  return map(fragCoord, vec2(2.5 * scale));
}

void main(void) {
  out_color = vec4(1.0);
  if (gl_FragCoord.x < resolution.x || gl_FragCoord.y > resolution.y) {
    out_color = vec4(0.0);
  }
  float res = normal.z > normal.y ? map_outer(fragTexCoord.xy) : map_outer(fragTexCoord.xz);
  out_color = (normal.z > normal.y ? vec4(0.2, 0.3, 0.5, 1.0) : vec4(0.5, 0.3, 0.2, 0.5))*res;
}

