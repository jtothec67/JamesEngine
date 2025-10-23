#version 460

in vec2 v_TexCoord;

uniform sampler2D u_AlbedoMap;
uniform float u_AlphaCutoff;     // e.g. 0.5

void main()
{
    float alpha = texture(u_AlbedoMap, v_TexCoord).a;
    if (alpha < u_AlphaCutoff)
        discard;
}