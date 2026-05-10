#include "common.hlsli"

struct VS_INPUT
{
	float3 pos : POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	float viewDepth : TEXCOORD1;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 worldPos = mul(float4(input.pos, 1.0f), frame.matWorld);
	float4 viewPos = mul(worldPos, frame.matView);
    output.pos = mul(viewPos, frame.matProj);

	output.color = input.color;

#if defined(SKY_CLOUD)
	float4 tex = mul(float4(input.tex, 0.0f, 1.0f), texTransform.tex0);
	output.tex = tex.xy;
#else
	output.tex = input.tex;
#endif

	output.viewDepth = viewPos.z;

	return output;
}
