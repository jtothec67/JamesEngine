#version 330 core
in vec2 vUV;
layout(location = 0) out vec2 OutBRDF; // RG16F target recommended

// Utilities (Hammersley + GGX sampling)
float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u)  | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u)  | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u)  | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u)  | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 2^32
}
vec2 Hammersley(uint i, uint N) { return vec2(float(i)/float(N), RadicalInverse_VdC(i)); }

vec3 ImportanceSampleGGX(vec2 Xi, float roughness, vec3 N)
{
    float a = roughness * roughness;
    float phi = 2.0 * 3.14159265359 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta*cosTheta));

    // tangent space H
    vec3 T = normalize(abs(N.z) < 0.999 ? cross(N, vec3(0,0,1)) : cross(N, vec3(0,1,0)));
    vec3 B = cross(T, N);
    vec3 H = normalize(T * (sinTheta*cos(phi)) + B * (sinTheta*sin(phi)) + N * cosTheta);
    return H;
}

float GeometrySchlickGGX(float NdotX, float roughness)
{
    float r = roughness + 1.0;
    float k = (r*r) / 8.0;
    return NdotX / (NdotX * (1.0 - k) + k);
}
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}

void main()
{
    // Convention: x = NdotV, y = roughness
    float NdotV   = clamp(vUV.x, 0.0, 1.0);
    float rough   = clamp(vUV.y, 0.0, 1.0);

    // View vector in local frame (N = (0,0,1))
    vec3 N = vec3(0,0,1);
    vec3 V = vec3(sqrt(1.0 - NdotV*NdotV), 0.0, NdotV);

    const uint SAMPLE_COUNT = 1024u; // 512–2048; 1024 is a good default
    float A = 0.0; // scale term
    float B = 0.0; // bias term

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, rough, N);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        float NdotV = max(V.z, 0.0);

        if (NdotL > 0.0) {
            float G = GeometrySmith(NdotV, NdotL, rough);
            float G_Vis = (G * VdotH) / max(NdotH * NdotV, 1e-5);

            // Split-sum: accumulate terms to fit F=F0
            float Fc = pow(1.0 - VdotH, 5.0); // Schlick factor
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);

    OutBRDF = vec2(A, B);
}
