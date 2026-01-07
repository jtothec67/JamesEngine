#version 330 core

in vec2 vUV;
out vec3 OutColor;

uniform sampler2D u_LowRes;
uniform sampler2D u_HighRes;
uniform float u_LowStrength;  // usually 1.0

void main()
{
    vec3 low  = texture(u_LowRes,  vUV).rgb;
    vec3 high = texture(u_HighRes, vUV).rgb;
    OutColor = high + low * u_LowStrength;
}
