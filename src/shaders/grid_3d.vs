#version 330

precision mediump float;

in vec3 vertexPosition;
in vec3 vertexColor;
in float z;

out vec3 fragTexCoord;
out vec3 normal;

uniform mat4 m_mvp;

void main()
{
    fragTexCoord = vertexPosition.xyz;
    normal = vertexColor.xyz;
    vec4 res = (m_mvp * vec4(vertexPosition.xyz, 1.0));
    gl_Position = vec4(res.xy, z*res.w, res.w);
}
