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

    float soft = clamp((luminance - u_BloomThreshold) / u_BloomKnee, 0.0, 1.0);
    float contribution = max(soft, step(u_BloomThreshold + u_BloomKnee, luminance));

    OutColor = color * contribution;
}
