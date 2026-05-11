#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
Texture2D txCharacterShadow : register(t4);

SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);
SamplerState sampler4 : register(s4);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
    float viewDepth : TEXCOORD2;
    float2 shadowUV : TEXCOORD3;
};

float4 ApplyFog(float4 color, float depth)
{
    if (fogEnable == 0 || fogEnd <= fogStart)
        return color;

    float fogFactor = saturate((fogEnd - depth) / (fogEnd - fogStart));
    color.rgb = lerp(fogColor.rgb, color.rgb, fogFactor);
    return color;
}


float4 RenderProjectedShadow(PS_INPUT input)
{
    if (input.shadowUV.x < 0.0f || input.shadowUV.x > 1.0f || input.shadowUV.y < 0.0f || input.shadowUV.y > 1.0f)
        discard;

    float4 shadow = txCharacterShadow.Sample(sampler4, input.shadowUV);

    if (shadow.r > 0.98f && shadow.g > 0.98f && shadow.b > 0.98f)
        discard;

    shadow.a = textureFactor.a;
    return shadow;
}

float4 RenderMesh(PS_INPUT input)
{
    if (useTexture0 == 0 && useTexture1 == 0)
        return textureFactor;

    float4 texColor0 = txDiffuse0.Sample(sampler0, input.tex0);
    float4 result = texColor0 * input.color;

#if defined(HAS_TEX2) && !defined(MESH_SPECULAR)
    float4 texColor1 = txDiffuse1.Sample(sampler1, input.tex1);
    result.rgb *= texColor1.rgb;
    result.a *= texColor1.a;
#endif

#if defined(MESH_SPECULAR)
    float4 specTex = txDiffuse1.Sample(sampler1, input.tex1);

    float specMask = saturate(result.a - 0.006f);
    specMask *= 1.0f - saturate((result.a - 0.8f) * 5.0f);

    float specLum = dot(specTex.rgb, float3(0.299f, 0.587f, 0.114f));

    result.rgb = saturate(result.rgb + specMask * specLum);
#endif

    if (alphaTestEnable != 0 && result.a < alphaRef / 255.0f)
        discard;

    return ApplyFog(result, input.viewDepth);
}

float4 main(PS_INPUT input) : SV_Target
{
#ifdef MESH_PROJECTED_SHADOW
    return RenderProjectedShadow(input);
#else
    return RenderMesh(input);
#endif
}
