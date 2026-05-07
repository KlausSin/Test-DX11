#include "common.hlsli"

Texture2D txDiffuse0 : register(t0);
SamplerState sampler0 : register(s0);

struct PS_INPUT
{
	float4 pos : SV_POSITION;
	float4 color : COLOR0;
	float2 tex : TEXCOORD0;
	float viewDepth : TEXCOORD1;
};

float4 main(PS_INPUT input) : SV_Target
{
	float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
	return texColor * input.color;
}