#version 460

in vec2 v_TexCoord;

uniform sampler2D u_Texture;
uniform float u_AlphaCutoff;     // e.g. 0.5
uniform int u_UseAlphaCutoff;    // 1 = enabled, 0 = disabled

void main()
{
    if (u_UseAlphaCutoff == 1)
    {
        float alpha = texture(u_Texture, v_TexCoord).a;
        if (alpha < u_AlphaCutoff)
            discard;
    }

    // No color output — depth is written automatically
}
