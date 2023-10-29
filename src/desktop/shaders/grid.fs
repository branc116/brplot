#version 330

precision mediump float;

//x, y, width, height
uniform vec4 resolution;
uniform float time;
uniform vec2 mouse;
uniform vec2 zoom;
uniform vec2 offset;
uniform vec2 screen;

in vec2 uv;
in vec4 color;
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
    vec2 cPos = ( fragCoord - .5*resolution.zw ) / resolution.w;
    return map(cPos, zoom, offset);
}

#define AA 1

void main(void) {
    vec2 fragCoord = gl_FragCoord.xy - (resolution.xy * vec2(1, -1.));
    fragCoord.y -= screen.y - resolution.w;
    if (fragCoord.y < 2. || (fragCoord.y + 2.) > resolution.w || fragCoord.x - 2. < 0. || (fragCoord.x + 2.) > resolution.z) {
      out_color = vec4(1.0);
    } else {
      float c = AA*2+1;
      float res = 0;
      for( float x = -AA ; x <= AA ; ++x)
        for( float y = -AA ; y <= AA ; ++y)
          res += map_outer(fragCoord+vec2(x, y)/(AA+1));
      res /= c*c;
      out_color = vec4(0.2, 0.3, 0.5, 1.0)*res;
    }
}
