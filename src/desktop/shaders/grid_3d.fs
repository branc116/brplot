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

in vec4 color;
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
    vec2 baseMajor = baseMinor + 1;
    vec2 divs = pow(vec2(10), baseMinor);
    vec2 divsMajor = pow(vec2(10), baseMajor);
    //cPos *= zoom_level;
    //cPos += offset.xy;
    vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
    vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;

    vec2 to = divs/(80. * (1. + 10.*(1. - 1.*fr)));
    //float f = 1.0 + length(cPos - eye.xy) / abs(eye.z) / 1.0;
    vec2 f = normal.z > normal.y ?
       1.0 + abs(cPos - eye.xy) / abs(eye.z) :
       1.0 + abs(cPos - eye.xz) / abs(eye.y);
    vec2 d = 1. - smoothstep(abs(mcPosd), vec2(0.0), to * f);
    vec2 dM = 1. - smoothstep(abs(mcPosdM), vec2(0.0), to * 3. * f);
    float k = 20.;

    return max(max(d.x, d.y), max(dM.x, dM.y));
}

float map_outer(vec2 fragCoord) {
    return map(fragCoord, vec2(2.5 * length(eye.xyz - fragTexCoord)));
}

void main(void) {
  if (gl_FragCoord.x < resolution.x || gl_FragCoord.y > resolution.y) {
    out_color = vec4(0.0);
    return;
  }
  float res = normal.z > normal.y ? map_outer(fragTexCoord.xy) : map_outer(fragTexCoord.xz);
  out_color = (normal.z > normal.y ? vec4(0.2, 0.3, 0.5, 1.0) : vec4(0.5, 0.3, 0.2, 1.0))*res;
}

