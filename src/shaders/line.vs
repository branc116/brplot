#version 330

precision mediump float;

in float vertexX;
in float vertexY;
in vec3 vertexColor;
// This is not normal. This is dx, dy, and length of the line.
in vec2 delta;

out float edge_dist;
out vec3 color;

uniform vec2 offset;
uniform vec2 zoom;
uniform vec2 screen;

void main(void)
{
    color = vertexColor;
    // TODO: make this uniform.
    float thick = 0.05;
    vec2 tg = delta;
    vec2 position = vec2(vertexX, vertexY);
    float dir = 1.0;
    if (color.b > 1.5) {
      dir = -1.0;
      color.b -= 2.0;
    }
    edge_dist = dir;
    color = abs(color);

    vec2 normal = -dir * normalize(tg.yx * zoom * vec2(-1.0, 1.0));
    vec2 dif = normal * thick;
    position -= dif * max(zoom * 0.1, position / 7e5);

    float aspect = 2.0;

    vec2 fact = screen.xy / screen.yy;
    vec2 uv = position * aspect;
    uv -= offset * 2.;
    uv /= zoom * fact;
    gl_Position = vec4(uv, 0.98, 1.0);
}

