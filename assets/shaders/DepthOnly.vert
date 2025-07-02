#version 460

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 u_Model;
uniform mat4 u_LightSpaceMatrix;

out vec2 v_TexCoord;

void main()
{
    v_TexCoord = aTexCoord;
    gl_Position = u_LightSpaceMatrix * u_Model * vec4(aPos, 1.0);
}
