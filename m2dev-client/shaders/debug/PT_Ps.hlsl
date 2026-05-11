#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
    float4 baseTex = txDiffuse0.Sample(sampler0, input.tex0);

#if defined(TERRAIN_BASE)
    float4 maskTex = txDiffuse1.Sample(sampler1, input.tex1);

    float4 result = baseTex;
    result.a = baseTex.a * maskTex.a;

    return result;

#elif defined(TERRAIN_SPLAT)
    return baseTex * textureFactor;
    
#elif defined(MINIMAP_MARK)
    if (alphaTestEnable != 0 && baseTex.a * 255.0f < alphaRef)
        discard;

    return float4(baseTex.rgb * input.color.rgb, baseTex.a);

#else
    return baseTex * input.color;
#endif
}