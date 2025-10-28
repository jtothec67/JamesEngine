#version 330 core
in vec2 vUV;
layout(location=0) out vec4 OutPrefilter;

uniform samplerCube u_SkyBox;
uniform mat3  u_FaceBasis;
uniform float u_Roughness;
uniform float u_SkyboxMaxLOD;

const float PI = 3.14159265359;

// Hammersley sequence
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

// GGX importance sampling of the HALF vector around N
vec3 ImportanceSampleGGX(vec2 Xi, float rough, vec3 N)
{
    float a  = rough*rough;
    float a2 = a*a;

    float phi = 2.0*PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a2 - 1.0) * Xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta*cosTheta));

    vec3 T = normalize(abs(N.z) < 0.999 ? cross(N, vec3(0,0,1)) : cross(N, vec3(0,1,0)));
    vec3 B = cross(T, N);
    return normalize(T*(sinTheta*cos(phi)) + B*(sinTheta*sin(phi)) + N*cosTheta);
}

// GGX normal distribution
float D_GGX(float NdotH, float rough)
{
    float a  = rough*rough;
    float a2 = a*a;
    float d  = (NdotH*NdotH)*(a2 - 1.0) + 1.0;
    return a2 / (PI * d * d);
}

void main()
{
    // Direction represented by this texel of the cube face
    vec3 N = normalize(u_FaceBasis * vec3(vUV, 1.0));

    // Split-sum prefilter uses V = N
    vec3 V = N;

    const uint SAMPLE_COUNT = 1024u;
    vec3  acc         = vec3(0.0);
    float totalWeight = 0.0;

    // Solid angle per texel of the base level of the source cube
    float baseSize = float(textureSize(u_SkyBox, 0).x);
    float omegaP   = 4.0 * PI / (6.0 * baseSize * baseSize);

    for(uint i=0u; i<SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);

        // Sample half-vector H around N, reflect view to get L
        vec3 H = ImportanceSampleGGX(Xi, u_Roughness, N);
        vec3 L = normalize(reflect(-V, H));

        float NdotL = max(dot(N, L), 0.0);
        if(NdotL > 0.0)
        {
            float NdotH = max(dot(N, H), 0.0);
            float VdotH = max(dot(V, H), 0.0);

            float D   = D_GGX(NdotH, u_Roughness);
            float pdf = max(D * NdotH / max(4.0 * VdotH, 1e-6), 1e-6);

            // Choose mip from sample footprint
            float omegaS = 1.0 / (float(SAMPLE_COUNT) * pdf);
            float mip    = 0.5 * log2(omegaS / omegaP);
            mip = clamp(mip, 0.0, u_SkyboxMaxLOD);

            acc         += textureLod(u_SkyBox, L, mip).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    vec3 prefiltered = (totalWeight > 0.0) ? acc / totalWeight : texture(u_SkyBox, N).rgb;
    OutPrefilter = vec4(prefiltered, 1.0);
}
