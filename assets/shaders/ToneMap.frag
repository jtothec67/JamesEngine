#version 460

in vec2 vUV;

uniform sampler2D u_HDRScene;  // hdrColorTex
uniform float u_Exposure;      // e.g. 1.0–2.0 as a start

out vec4 FragColor;

void main()
{
    vec3 hdr = texture(u_HDRScene, vUV).rgb;

    // Apply exposure
    vec3 x = hdr * u_Exposure;

    // Reinhard tone map
    vec3 mapped = x / (1.0 + x);

    FragColor = vec4(mapped, 1.0);
}