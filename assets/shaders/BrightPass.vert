#version 330 core
layout(location=0) in vec3 a_Position;
layout(location=1) in vec2 a_TexCoord;
out vec2 vUV;

void main()
{
    vec2 clip = vec2(a_Position) * 2.0 - 1.0;
    gl_Position = vec4(clip, 0.0, 1.0);

    vUV = a_TexCoord;
}