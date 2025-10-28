#version 330 core
in vec2 vUV;
layout(location=0) out vec4 OutIrradiance;

uniform samplerCube u_SkyBox;
uniform mat3 u_FaceBasis;

const float PI = 3.14159265359;

// Hammersley + cosine hemisphere
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N){ return vec2(float(i)/float(N), RadicalInverse_VdC(i)); }

vec3 CosineSampleHemisphere(vec2 Xi)
{
    float phi      = 2.0*PI * Xi.x;
    float cosTheta = sqrt(1.0 - Xi.y);
    float sinTheta = sqrt(Xi.y);
    return vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);
}

mat3 MakeTBN(vec3 N)
{
    vec3 up = (abs(N.z) < 0.999) ? vec3(0,0,1) : vec3(0,1,0);
    vec3 T  = normalize(cross(up, N));
    vec3 B  = cross(N, T);
    return mat3(T, B, N);
}

void main()
{
    // Direction represented by this texel of the cube face
    vec3 N = normalize(u_FaceBasis * vec3(vUV, 1.0));

    // Cosine-weighted convolution over the hemisphere about N
    const uint SAMPLE_COUNT = 256u;
    mat3 TBN = MakeTBN(N);

    vec3 acc = vec3(0.0);
    for(uint i=0u; i<SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 Ls = CosineSampleHemisphere(Xi);
        vec3 L  = normalize(TBN * Ls);
        acc += texture(u_SkyBox, L).rgb;
    }

    vec3 irradiance = acc / float(SAMPLE_COUNT);
    OutIrradiance = vec4(irradiance, 1.0);
}
