#version 460

in vec2 v_TexCoord;

uniform sampler2D u_AlbedoMap;
uniform float u_AlphaCutoff;

void main()
{
    float lod = textureQueryLod(u_AlbedoMap, v_TexCoord).x; // Hardware-chosen LOD
    float clampedLod = min(lod, 3.0);                       // Cap at mip 3 (specifically for fences)
    vec4 albedoTex = textureLod(u_AlbedoMap, v_TexCoord, clampedLod);
    float alpha = albedoTex.a;
    if (alpha < u_AlphaCutoff)
        discard;
}