#version 330

precision mediump float;

in vec3 vertexPosition;
in vec3 vertexNormal;

out vec3 normal;

uniform mat4 m_mvp;

void main(void) {
  normal = vertexNormal;
  gl_Position = m_mvp * vec4(vertexPosition, 1.0);
}

