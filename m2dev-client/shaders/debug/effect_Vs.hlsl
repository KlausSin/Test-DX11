#include "common.hlsli"

struct VS_INPUT
{
	float3 pos : POSITION;
	float2 tex : TEXCOORD0;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex : TEXCOORD0;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 worldPos = mul(float4(input.pos, 1.0f),frame. matWorld);
	float4 viewPos = mul(worldPos, frame.matView);

	output.pos = mul(viewPos, frame.matProj);
	output.tex = input.tex;

	return output;
}
