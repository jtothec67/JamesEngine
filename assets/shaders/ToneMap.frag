#version 460

in vec2 vUV;

uniform sampler2D u_HDRScene;
uniform float u_Exposure;

out vec4 FragColor;

void main()
{
    vec3 hdr = texture(u_HDRScene, vUV).rgb;

    // Apply exposure
    vec3 x = hdr * u_Exposure;

    // Reinhard luminance
    float L = dot(x, vec3(0.2126, 0.7152, 0.0722));
    float Lmapped = L / (1.0 + L);
    float scale = (L > 0.0) ? (Lmapped / L) : 0.0;
    vec3 mapped = x * scale;

    FragColor = vec4(mapped, 1.0);
}