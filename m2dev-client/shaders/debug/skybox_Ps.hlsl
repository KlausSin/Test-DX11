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
#if defined(SKY_USE_TEXTURE)
	float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
	return texColor;
#elif defined(SKY_USE_DIFFUSE)
	return input.color;
#elif defined(SKY_CLOUD)
	float4 texColor = txDiffuse0.Sample(sampler0, input.tex);
	return texColor;
#else
	return float4(1.0f, 1.0f, 1.0f, 1.0f);
#endif
}
