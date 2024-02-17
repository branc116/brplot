#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;
out vec2 fragTexCoord;
out vec4 fragColor;
uniform mat4 mvp;
uniform mat4 m_mvp;
uniform mat4 m_view;
uniform mat4 m_projection;
uniform vec4 resolution;
uniform vec2 screen;
uniform float time;
void main()
{
    fragTexCoord = vertexPosition.zz;
    fragColor = vertexColor;
    gl_Position = vec4(vertexPosition.xy, 0.0, 1.0);
}
