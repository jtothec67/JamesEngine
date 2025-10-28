#version 330 core
in vec2 vUV;
layout(location=0) out vec2 OutBRDF;

// ammersley + GGX helpers
float RadicalInverse_VdC(uint bits)
{ 
    bits=(bits<<16u)|(bits>>16u);
    bits=((bits&0x55555555u)<<1u)|((bits&0xAAAAAAAAu)>>1u);
    bits=((bits&0x33333333u)<<2u)|((bits&0xCCCCCCCCu)>>2u);
    bits=((bits&0x0F0F0F0Fu)<<4u)|((bits&0xF0F0F0F0u)>>4u);
    bits=((bits&0x00FF00FFu)<<8u)|((bits&0xFF00FF00u)>>8u);
    return float(bits)*2.3283064365386963e-10;
}
vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i)/float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, float rough, vec3 N)
{
    float a=rough*rough;
    float phi=6.28318530718*Xi.x;
    float cosTheta=sqrt((1.0-Xi.y)/(1.0+(a*a-1.0)*Xi.y));
    float sinTheta=sqrt(max(0.0,1.0-cosTheta*cosTheta));
    vec3 T=normalize(abs(N.z)<0.999?cross(N,vec3(0,0,1)):cross(N,vec3(0,1,0)));
    vec3 B=cross(T,N);
    return normalize(T*(sinTheta*cos(phi)) + B*(sinTheta*sin(phi)) + N*cosTheta);
}

// IBL visibility approximation (use this for the BRDF LUT)
float GeometrySchlickGGX_IBL(float NdotX, float rough)
{
    float a=rough*rough;      // alpha^2
    float k=0.5*a;            // IBL form
    return NdotX/(NdotX*(1.0-k)+k);
}
float GeometrySmith_IBL(float NdotV, float NdotL, float rough)
{
    return GeometrySchlickGGX_IBL(NdotV, rough) * GeometrySchlickGGX_IBL(NdotL, rough);
}

void main()
{
    // Convention: x = NdotV, y = roughness, both in [0,1]
    float NdotV = clamp(vUV.x, 0.0, 1.0);
    float rough = clamp(vUV.y, 0.0, 1.0);

    vec3 N=vec3(0,0,1);
    vec3 V=vec3(sqrt(max(0.0,1.0-NdotV*NdotV)),0.0,NdotV);

    const uint SAMPLE_COUNT=1024u;
    float A=0.0, B=0.0;

    for(uint i=0u;i<SAMPLE_COUNT;++i)
    {
        vec2 Xi=Hammersley(i, SAMPLE_COUNT);
        vec3 H=ImportanceSampleGGX(Xi, rough, N);
        vec3 L=normalize(2.0*dot(V,H)*H - V);

        float NdotL=max(L.z,0.0);
        float NdotH=max(H.z,0.0);
        float VdotH=max(dot(V,H),0.0);
        float NdotV2=max(V.z,0.0);

        if(NdotL>0.0)
        {
            float G=GeometrySmith_IBL(NdotV2, NdotL, rough);
            float Gvis=(G*VdotH)/max(NdotH*NdotV2,1e-5);
            float Fc=pow(1.0-VdotH,5.0);
            A+=(1.0-Fc)*Gvis;
            B+=Fc*Gvis;
        }
    }

    OutBRDF=vec2(A,B)/float(SAMPLE_COUNT);
}
