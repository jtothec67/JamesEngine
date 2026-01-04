#version 330 core

in vec2 vUV;
layout(location = 0) out vec3 OutColor;

uniform sampler2D u_Scene; // HDR scene
uniform float u_BloomThreshold;
uniform float u_BloomKnee;

void main()
{
    vec3 color = texture(u_Scene, vUV).rgb;
    float luminance = dot(color, vec3(0.2126, 0.7152, 0.0722));

    float x = max(luminance - u_BloomThreshold, 0.0);

    float softExcess = (x * x) / (x + u_BloomKnee);
    float hardExcess = max(luminance - (u_BloomThreshold + u_BloomKnee), 0.0);
    float excess = softExcess + hardExcess;

    // Scale colour by excess luminance, preserving chroma
    OutColor = color * (excess / max(luminance, 1e-6));
}
