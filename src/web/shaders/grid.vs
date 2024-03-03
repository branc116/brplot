#version 300 es
in vec2 vertexPosition;

out vec2 fragTexCoord;
out vec2 glCoord;
uniform vec2 screen;

void main()
{
  glCoord = vertexPosition;
  fragTexCoord = vec2(screen.x/screen.y, 1.0)*(vertexPosition);
  gl_Position = vec4(vertexPosition.xy, 0.0, 1.0);
}
