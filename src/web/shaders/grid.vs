#version 100
attribute vec2 vertexPosition;

varying vec2 fragTexCoord;
varying vec2 glCoord;
uniform vec2 screen;

void main()
{
  glCoord = vertexPosition;
  fragTexCoord = vec2(screen.x/screen.y, 1.0)*(vertexPosition);
  gl_Position = vec4(vertexPosition.xy, 0.0, 1.0);
}
