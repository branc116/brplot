#version 330
in vec3 vertexPosition;
in vec3 vertexColor;

out vec3 fragTexCoord;
out vec4 fragColor;
out vec3 normal;

uniform mat4 m_mvp;

void main()
{
    fragTexCoord = vertexPosition.xyz;
    fragColor = vec4(vertexColor, 1.0);
    normal = vertexColor.xyz;
    gl_Position = m_mvp * vec4(vertexPosition.xyz, 1.0);
}
