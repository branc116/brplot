#version 330

in vec3 vertexPosition;
in vec3 vertexColor;
in vec3 vertexNormal;

out vec3 pos;
out vec3 color;
out vec3 normal;

uniform mat4 m_mvp;

void main(void)
{
  pos = vertexPosition;
  color = vertexColor;
  normal = normalize(vertexNormal);
  gl_Position = m_mvp * vec4(vertexPosition, 1.0);
}

