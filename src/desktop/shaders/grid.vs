#version 330

precision mediump float;

in vec2 vertexPosition;

out vec2 fragTexCoord;

uniform vec2 screen;

void main() {
  fragTexCoord = vec2(screen.x/screen.y, 1.0)*(vertexPosition);
  gl_Position = vec4(vertexPosition.xy, 0.0, 1.0);
}
