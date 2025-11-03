#version 330 core
in vec2 vUV;
layout(location=0) out float OutAO;

uniform sampler2D u_Depth;
uniform mat4 u_Proj;
uniform mat4 u_InvProj;
uniform vec2 u_InvResolution;     // 1/width, 1/height
uniform float u_Radius;           // e.g. 0.6
uniform float u_Bias;             // e.g. 0.005
uniform float u_Power;            // e.g. 1.4

const int NUM_SAMPLES = 16;
const vec3 s_Kernel[NUM_SAMPLES] = vec3[]( // Hard coded for now
    vec3( 0.170,  0.130, 0.120),
    vec3(-0.230,  0.040, 0.180),
    vec3( 0.090, -0.240, 0.220),
    vec3( 0.320, -0.110, 0.260),
    vec3(-0.180, -0.310, 0.300),
    vec3(-0.350,  0.220, 0.340),
    vec3( 0.260,  0.330, 0.380),
    vec3( 0.420, -0.250, 0.420),
    vec3(-0.420, -0.120, 0.460),
    vec3( 0.120,  0.450, 0.500),
    vec3( 0.500,  0.100, 0.540),
    vec3(-0.280,  0.440, 0.580),
    vec3( 0.430,  0.300, 0.620),
    vec3(-0.520, -0.050, 0.700),
    vec3(-0.220, -0.520, 0.760),
    vec3( 0.160,  0.560, 0.820)
);

vec3 ReconstructVS(vec2 uv, float depth)
{
    float z = depth * 2.0 - 1.0;
    vec4 ndc = vec4(uv * 2.0 - 1.0, z, 1.0);
    vec4 v = u_InvProj * ndc;
    return v.xyz / v.w;
}

vec3 ReconstructVSNormal(vec2 uv)
{
    vec2 du = vec2(u_InvResolution.x, 0.0);
    vec2 dv = vec2(0.0, u_InvResolution.y);

    float dL = texture(u_Depth, uv - du).r;
    float dR = texture(u_Depth, uv + du).r;
    float dD = texture(u_Depth, uv - dv).r;
    float dU = texture(u_Depth, uv + dv).r;

    vec3 pL = ReconstructVS(uv - du, dL);
    vec3 pR = ReconstructVS(uv + du, dR);
    vec3 pD = ReconstructVS(uv - dv, dD);
    vec3 pU = ReconstructVS(uv + dv, dU);

    vec3 dx = pR - pL;
    vec3 dy = pU - pD;

    vec3 n = cross(dx, dy);
    float len2 = dot(n, n);
    if (len2 < 1e-12)
    {
        return vec3(0.0, 0.0, 1.0);
    }
    return normalize(n);
}

// cheap repeatable hash -> [0,1)
float hash12(vec2 p)
{
    vec3 p3 = fract(vec3(p.x, p.y, p.x) * 0.1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// build an orthonormal basis from a normal
mat3 basisFromNormal(vec3 n)
{
    vec3 up = (abs(n.z) < 0.999) ? vec3(0, 0, 1) : vec3(0, 1, 0);
    vec3 t = normalize(cross(up, n));
    vec3 b = cross(n, t);
    return mat3(t, b, n);
}

// rotate around +Z (kernel local) by angle (for per-pixel jitter)
mat3 rotAroundZ(float a)
{
    float c = cos(a), s = sin(a);
    return mat3(
        c, -s, 0.0,
        s,  c, 0.0,
        0.0, 0.0, 1.0
    );
}

void main()
{
    float d = texture(u_Depth, vUV).r;

    if (d >= 1.0)
    {
        OutAO = 1.0;
        return;
    }

    vec3 P = ReconstructVS(vUV, d);
    vec3 N = ReconstructVSNormal(vUV);

    mat3 TBN = basisFromNormal(N);
    float angle = hash12(vUV / u_InvResolution) * 6.2831853; // 2PI
    mat3 Rz = rotAroundZ(angle);

    float occl = 0.0;
    float radiusVS = u_Radius * clamp(-P.z * 0.0015, 0.5, 3.0);

    for (int i = 0; i < NUM_SAMPLES; ++i)
    {
        vec3 sampleVS = P + TBN * (Rz * (s_Kernel[i] * radiusVS));

        vec4 cs = u_Proj * vec4(sampleVS, 1.0);
        vec2 uvS = cs.xy / cs.w * 0.5 + 0.5;

        if (uvS.x < 0.0 || uvS.x > 1.0 || uvS.y < 0.0 || uvS.y > 1.0)
        {
            continue;
        }

        float dS = texture(u_Depth, uvS).r;
        vec3 Q = ReconstructVS(uvS, dS);

        float range = smoothstep(0.0, 1.0, radiusVS / abs(P.z - Q.z));
        if (Q.z > sampleVS.z + u_Bias)
        {
            occl += range;
        }
    }

    float ao = 1.0 - occl / float(NUM_SAMPLES);
    OutAO = pow(ao, u_Power);
}
