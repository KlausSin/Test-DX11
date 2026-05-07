#include "common.hlsli"

struct VS_INPUT
{
	float3 pos : POSITION;
	float3 normal : NORMAL;
};

struct VS_OUTPUT
{
	float4 pos : SV_POSITION;
	float2 tex0 : TEXCOORD0;
	float2 tex1 : TEXCOORD1;
	float viewDepth : TEXCOORD2;
};

VS_OUTPUT main(VS_INPUT input)
{
	VS_OUTPUT output;

	float4 worldPos = mul(float4(input.pos, 1.0f), matWorld);
	float4 viewPos = mul(worldPos, matView);

	output.pos = mul(viewPos, matProj);

	float4 uv0 = mul(float4(viewPos.xyz, 1.0f), matTexTransform0);
	float4 uv1 = mul(float4(viewPos.xyz, 1.0f), matTexTransform1);

	output.tex0 = uv0.xy;
	output.tex1 = uv1.xy;
	output.viewDepth = viewPos.z;

	return output;
}
