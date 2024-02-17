#version 330

precision mediump float;

//x, y, width, height
uniform vec4 resolution;
uniform float time;
uniform vec2 mouse;
uniform vec2 zoom;
uniform vec2 offset;
uniform vec2 screen;

uniform mat4 m_mvp;
uniform mat4 m_view;
uniform mat4 m_projection;

in vec2 uv;
in vec4 color;
in vec2 fragTexCoord;

out vec4 out_color;

vec2 log10(vec2 f) {
    return log(f) / log(10.0);
}

float map(vec2 cPos, vec2 zoom_level, vec2 offset) {
    vec2 fact = log10(zoom_level * 4.5);
    vec2 fr = fract(fact);
    vec2 baseMinor = floor(fact) - 1.0;
    vec2 baseMajor = baseMinor + 1;
    vec2 divs = pow(vec2(10), baseMinor);
    vec2 divsMajor = pow(vec2(10), baseMajor);
    cPos *= zoom_level;
    cPos += offset;
    vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
    vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;

    vec2 to = divs/(80. * (1. + 5.*(1. - 1.*fr)));
    vec2 d = 1. - smoothstep(abs(mcPosd), vec2(0.0), to);
    vec2 dM = 1. - smoothstep(abs(mcPosdM), vec2(0.0), to*4);
    float k = 20.;

    return max(max(d.x, d.y), max(dM.x, dM.y));
}

float map_outer(vec2 fragCoord) {
  return length(fragCoord) > 0.1 ? 1.0 : 0.0;
    vec2 cPos = fragCoord.xy;
    vec4 p = vec4(cPos, .0, 1.0);
    p = m_mvp * p;
    if (p.z < 0.) return 0.0;
    p /= p.w;
    if (p.z < 0.001) return 0.0;
    return map(p.xy, vec2(1.0, 1.0), vec2(0.0, 0.0));
}

#define AA 1

void main(void) {
  vec2 uv = (((gl_FragCoord.xy - resolution.xy)/screen) - (resolution.zw/screen)/2) / vec2(resolution.w/resolution.z, 1.0);
  //uv /= vec2(resolution.w/resolution.z, 1.0);
  //uv -= vec2(resolution.z/screen.x/2,  -resolution.w/screen.y/2);
  float res = map_outer(uv);
  out_color = vec4(0.2, 0.3, 0.5, 1.0)*res;
}
