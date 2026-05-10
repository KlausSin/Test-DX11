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
    float2 tex1 : TEXCOORD2;
    float viewDepth : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
    float4 texColor0 = txDiffuse0.Sample(sampler0, input.tex0);
    float4 stage0 = texColor0 * input.color;
    float4 result = stage0;

#if defined(HAS_TEX2) && !defined(MESH_SPECULAR)
    float4 texColor1 = txDiffuse1.Sample(sampler1, input.tex1);
    result.rgb = stage0.rgb * texColor1.rgb;
#endif

#if defined(MESH_SPECULAR)
    float4 texColor1 = txDiffuse1.Sample(sampler1, input.tex1);

    float specMask = saturate(stage0.a - 0.006f);
    specMask *= 1.0f - saturate((stage0.a - 0.8f) * 5.0f);

    result.rgb = saturate(stage0.rgb + specMask * texColor1.rgb);
#endif

    result.a = stage0.a;

#if defined(MESH_ALPHA_TEST)
    if (result.a < alphaRef / 255.0f)
        discard;
#endif

    if (fogEnable && fogEnd > fogStart)
    {
        float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
        result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
    }

    return result;
}