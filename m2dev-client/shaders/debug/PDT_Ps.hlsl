
#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
Texture2D txDiffuse1 : register(t1);
SamplerState sampler0 : register(s0);
SamplerState sampler1 : register(s1);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex : TEXCOORD0;
    float viewDepth : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
    float4 result;

#if defined(BLEND_UI_DIFFUSE)
    // SELECTARG1 = diffuse only, no texture
    result = input.color;

#elif defined(BLEND_UI_TEX)
    // SELECTARG1 = texture only, alpha modulated by vertex alpha
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result = float4(texColor.rgb, texColor.a * input.color.a);

#elif defined(BLEND_MODULATE)
    // tex * diffuse
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result = texColor * input.color;

#elif defined(BLEND_MODULATE2X)
    // tex * diffuse * 2
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result = texColor * input.color * 2.0f;
    result.a = texColor.a * input.color.a;

#elif defined(BLEND_ADD)
    // tex + diffuse (diffuse color via vertex color)
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result.rgb = saturate(texColor.rgb + input.color.rgb);
    result.a = texColor.a * input.color.a;

#elif defined(BLEND_SELECTARG2)
    // SELECTARG2 = arg2 = diffuse, alpha from texture
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result.rgb = input.color.rgb;
    result.a = texColor.a;

#elif defined(BLEND_STAGE1_MODULATE)
    // stage0: tex0 * diffuse, stage1: * tex1
    float4 texColor0 = txDiffuse0.Sample(sampler0, input.tex);
    float4 texColor1 = txDiffuse1.Sample(sampler1, input.tex);
    float4 stage0 = texColor0 * input.color;
    result.rgb = stage0.rgb * texColor1.rgb;
    result.a = stage0.a * texColor1.a;

#elif defined(BLEND_TEXFACTOR)
    // textureFactor blend: tex * vertex color (vertex color carries factor)
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result.rgb = texColor.rgb * input.color.rgb;
    result.a = texColor.a * input.color.a;

#else
    // fallback: MODULATE
    float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
    result = texColor * input.color;
#endif

    // Fog
    if (fogEnable && fogEnd > fogStart)
    {
        float fogFactor = saturate((fogEnd - input.viewDepth) / (fogEnd - fogStart));
        result.rgb = lerp(fogColor.rgb, result.rgb, fogFactor);
    }

    return result;
}
