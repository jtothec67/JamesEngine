#version 330 core

in vec2 vUV;
layout(location = 0) out vec3 OutColor;

uniform sampler2D u_Source;
uniform vec2 u_InvResolution; // (1.0 / textureWidth, 1.0 / textureHeight) for this texture
uniform vec2 u_Direction; // (1,0) = horizontal pass, (0,1) = vertical pass

// 9-tap Gaussian, normalized
const float w[5] = float[5](
    0.227027,
    0.1945946,
    0.1216216,
    0.054054,
    0.016216
);

void main()
{
    vec2 stepUV = u_Direction * u_InvResolution;

    vec3 color = texture(u_Source, vUV).rgb * w[0];

    for (int i = 1; i < 5; ++i)
    {
        vec2 offset = stepUV * float(i);
        color += texture(u_Source, vUV + offset).rgb * w[i];
        color += texture(u_Source, vUV - offset).rgb * w[i];
    }

    OutColor = color;
}