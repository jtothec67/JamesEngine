#version 330 core

in vec2 vUV;
out vec3 OutColor;

uniform sampler2D u_RawTexture;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(u_RawTexture, 0));
    OutColor = texture(u_RawTexture, vUV + texelSize * 0.5).rgb;
}