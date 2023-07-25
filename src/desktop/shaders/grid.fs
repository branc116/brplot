#version 330

precision mediump float;

//x, y, width, height
uniform vec4 resolution;
uniform float time;
uniform vec2 mouse;
uniform vec2 zoom;
uniform vec2 offset;

in vec2 uv;
in vec4 color;
out vec4 out_color;

float log10(float f) {
    return log(f) / log(10.0);
}

float map(vec2 cPos, vec2 zoom_level, vec2 offset) {
    vec2 fact = vec2(log10(zoom_level.x * 4.5), log10(zoom_level.y * 4.5));
    vec2 fr = vec2(fract(fact.x), fract(fact.y));
    vec2 baseMinor = floor(fact) - 1.0;
    vec2 baseMajor = baseMinor + 1;
    vec2 divs = vec2(pow(10, baseMinor.x), pow(10, baseMinor.y));
    vec2 divsMajor = vec2(pow(10, baseMajor.x), pow(10, baseMajor.y));
    cPos *= zoom_level;
    cPos += offset;
    vec2 mcPosd = mod(cPos + divs/2., divs) - divs/2.;
    vec2 mcPosdM = mod(cPos + divsMajor/2., divsMajor) - divsMajor/2.;

    vec2 to = divs/(80. * (1. + 5.*(1. - 1.*fr)));
    float yd = 1. - smoothstep(abs(mcPosd.y), 0.0, to.y);
    float xd = 1. - smoothstep(abs(mcPosd.x), 0.0, to.x);
    float ydM = 1. - smoothstep(abs(mcPosdM.y), 0.0, to.y*4);
    float xdM = 1. - smoothstep(abs(mcPosdM.x), 0.0, to.x*4);
    float k = 20.;
    return max(max(yd, xd), max(ydM, xdM));
}

void main(void) {
    vec2 fragCoord = gl_FragCoord.xy - resolution.xy;
    vec2 uv = fragCoord.xy / resolution.zw;
    vec2 cPos = ( fragCoord - .5*resolution.zw ) / resolution.w;
    out_color = vec4(0.2, 0.3, 0.5, 1.0)*vec4(map(cPos, zoom, offset));
    if (fragCoord.y - 2. < 0. || fragCoord.y + 2. > resolution.w || fragCoord.x - 2. < 0. || (fragCoord.x + 2.) > resolution.z) {
      out_color = vec4(1.0);
    }
}
