#version 330

precision mediump float;

//x, y, width, height
uniform vec4 resolution;
uniform float time;
uniform vec2 mouse;
uniform vec2 zoom;
uniform vec3 offset;
uniform vec2 screen;

uniform vec3 eye;

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

float map(vec2 cPos, vec2 zoom_level) {
    vec2 fact = log10(zoom_level * 4.5);
    vec2 fr = fract(fact);
    vec2 baseMinor = floor(fact) - 1.0;
    vec2 baseMajor = baseMinor + 1;
    vec2 divs = pow(vec2(10), baseMinor);
    vec2 divsMajor = pow(vec2(10), baseMajor);
    //cPos *= zoom_level;
    //cPos += offset.xy;
    vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
    vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;

    vec2 to = divs/(80. * (1. + 5.*(1. - 1.*fr)));
    vec2 d = 1. - smoothstep(abs(mcPosd), vec2(0.0), to);
    vec2 dM = 1. - smoothstep(abs(mcPosdM), vec2(0.0), to*4);
    float k = 20.;

    return max(max(d.x, d.y), max(dM.x, dM.y));
}

vec2 map_outer(vec2 fragCoord) {
    vec2 cPos = fragCoord.xy;
    vec4 p = vec4(cPos, .0, 1.0);
    p = m_mvp * p;
    if (p.z < 0.) return vec2(0.0);
    p /= p.w;
    if (p.z < 0.001) return vec2(0);
    vec3 diff = p.xyz - offset;
    p.xyz += offset;
    if (length(p.xy) < 0.1) return vec2(p.z, 200.0);
    return vec2(p.z, map(p.xy, vec2(length(diff.xyz))));
}

#define AA 1

void main(void) {
  vec2 uv = (gl_FragCoord.xy - resolution.xy - resolution.zw*0.5)/screen.yy;
  vec2 res = map_outer(uv);
  //out_color = vec4(0.2, 0.3, 0.5, 1.0)*res;
  out_color = vec4(res.x/100., res.y, 0., 1.0);
}

