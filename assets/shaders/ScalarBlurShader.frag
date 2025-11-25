#version 330 core
in vec2 vUV;
layout(location = 0) out float OutValue;

uniform sampler2D u_Source;
uniform vec2 u_InvResolution; // (1.0 / textureWidth, 1.0 / textureHeight) for this texture
uniform vec2 u_Direction; // (1,0) = horizontal pass, (0,1) = vertical pass
uniform float u_StepScale; // Multiplier for blur radius — 1.0 = normal, >1.0 = wider blur

// 9-tap Gaussian weights (radius 4)
const float w[9] = float[9](
    0.027, 0.066, 0.122, 0.179, 0.207, 0.179, 0.122, 0.066, 0.027
);

void main()
{
    vec2 stepUV = u_Direction * u_InvResolution * u_StepScale;

    float sum = 0.0;

    // center sample
    sum += texture(u_Source, vUV).r * w[4];

    // mirrored taps around center
    sum += texture(u_Source, vUV + 1.0 * stepUV).r * w[5];
    sum += texture(u_Source, vUV - 1.0 * stepUV).r * w[3];

    sum += texture(u_Source, vUV + 2.0 * stepUV).r * w[6];
    sum += texture(u_Source, vUV - 2.0 * stepUV).r * w[2];

    sum += texture(u_Source, vUV + 3.0 * stepUV).r * w[7];
    sum += texture(u_Source, vUV - 3.0 * stepUV).r * w[1];

    sum += texture(u_Source, vUV + 4.0 * stepUV).r * w[8];
    sum += texture(u_Source, vUV - 4.0 * stepUV).r * w[0];

    OutValue = sum;
}
