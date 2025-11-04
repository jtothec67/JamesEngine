#version 330 core
in vec2 vUV;
layout(location=0) out float OutAO;

uniform sampler2D u_RawAO;
uniform vec2 u_InvResolution;      // (1/halfW, 1/halfH)
uniform vec2 u_Direction;      // (1,0)=horizontal, (0,1)=vertical

// 9-tap Gaussian weights (radius=4), sum=1
const float w[9] = float[9](
    0.027, 0.066, 0.122, 0.179, 0.207, 0.179, 0.122, 0.066, 0.027
);

void main()
{
    vec2 stepUV = u_Direction * u_InvResolution;

    float ao = 0.0;
    ao += texture(u_RawAO, vUV).r * w[4];

    ao += texture(u_RawAO, vUV + 1.0 * stepUV).r * w[5];
    ao += texture(u_RawAO, vUV - 1.0 * stepUV).r * w[3];

    ao += texture(u_RawAO, vUV + 2.0 * stepUV).r * w[6];
    ao += texture(u_RawAO, vUV - 2.0 * stepUV).r * w[2];

    ao += texture(u_RawAO, vUV + 3.0 * stepUV).r * w[7];
    ao += texture(u_RawAO, vUV - 3.0 * stepUV).r * w[1];

    ao += texture(u_RawAO, vUV + 4.0 * stepUV).r * w[8];
    ao += texture(u_RawAO, vUV - 4.0 * stepUV).r * w[0];

    OutAO = ao;
}
