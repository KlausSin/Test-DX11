#include "common.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos = mul(float4(input.pos, 1.0f), frame.matWorld);
    float4 viewPos = mul(worldPos, frame.matView);

    output.pos = mul(viewPos, frame.matProj);
#if defined(MINIMAP_MARK)
    output.color = textureFactor;
#else
    output.color = float4(1, 1, 1, 1);
#endif

    output.tex0 = input.tex;

#if defined(TERRAIN_BASE)
    float4 tc1 = mul(float4(worldPos.xy, 0.0f, 1.0f), texTransform.tex1);
    output.tex1 = tc1.xy;
#else
    output.tex1 = input.tex;
#endif

    return output;
}