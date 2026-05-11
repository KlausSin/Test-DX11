#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
Texture2D txSplatAlpha : register(t1);
Texture2D txShadowStatic : register(t2);
Texture2D txShadowCharacter : register(t3);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
SamplerState sampler2 : register(s2);
SamplerState sampler3 : register(s3);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
    float2 texShadowStatic : TEXCOORD2;
    float2 texShadowCharacter : TEXCOORD3;
    float viewDepth : TEXCOORD4;
};

float4 ApplyTerrainFog(float4 color, float depth)
{
    if (fogEnable && fogEnd > fogStart)
    {
        float fogFactor = saturate((fogEnd - depth) / (fogEnd - fogStart));
        fogFactor = lerp(fogFactor, 1.0f, 0.35f);
        color.rgb = lerp(fogColor.rgb, color.rgb, fogFactor);
    }

    return color;
}

float3 ApplyTerrainShadow(float3 color, PS_INPUT input)
{
    float3 staticShadow = txShadowStatic.Sample(sampler2, input.texShadowStatic).rgb;
    float3 characterShadow = txShadowCharacter.Sample(sampler3, input.texShadowCharacter).rgb;
    return color * staticShadow * characterShadow;
}

float4 main(PS_INPUT input) : SV_Target
{
    float4 colorTex = txDiffuse0.Sample(sampler0, input.tex0);
    float alpha = txSplatAlpha.Sample(sampler1, input.tex1).a;

    float4 result = colorTex * input.color;
    result.rgb = ApplyTerrainShadow(result.rgb, input);
    result.a = alpha * textureFactor.a;

    return ApplyTerrainFog(result, input.viewDepth);
}
