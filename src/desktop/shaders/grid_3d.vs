#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

out vec3 fragTexCoord;
out vec4 fragColor;
out vec3 normal;

uniform mat4 mvp;
uniform mat4 m_mvp;
uniform mat4 m_view;
uniform mat4 m_projection;
uniform vec4 resolution;
uniform vec2 screen;
uniform float time;
void main()
{
    fragTexCoord = vertexPosition.xyz;
    fragColor = vertexColor;
    normal = vertexColor.xyz;
    gl_Position = m_mvp * vec4(vertexPosition.xyz, 1.0);
}
