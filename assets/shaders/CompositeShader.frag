#version 330 core

in vec2 vUV;
layout(location = 0) out vec3 OutColor;

uniform sampler2D u_Scene;

uniform sampler2D u_Bloom;
uniform float u_BloomStrength;

void main()
{
    // Base shaded scene
    vec3 scene = texture(u_Scene, vUV).rgb;

    // Bloom contribution
    vec3 bloom = texture(u_Bloom, vUV).rgb;

    // Combine in HDR space
    vec3 hdr = scene + bloom * u_BloomStrength;

    OutColor = hdr;
}