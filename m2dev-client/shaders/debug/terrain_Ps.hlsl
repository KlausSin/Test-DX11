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
	float2 tex1 : TEXCOORD1;
	float viewDepth : TEXCOORD2;
};

float4 ApplyFog(float4 color, float depth)
{
	if (fogEnable && fogEnd > fogStart)
	{
		float fogFactor = saturate((fogEnd - depth) / (fogEnd - fogStart));
		color.rgb = lerp(fogColor.rgb, color.rgb, fogFactor);
	}

	return color;
}

float4 main(PS_INPUT input) : SV_Target
{
#if defined(TERRAIN_BASE)

	float4 colorTex = txDiffuse0.Sample(sampler0, input.tex0);
	float4 result = colorTex * input.color;

	return ApplyFog(result, input.viewDepth);

#elif defined(TERRAIN_SPLAT)

	float4 colorTex = txDiffuse0.Sample(sampler0, input.tex0);
	float4 alphaTex = txDiffuse1.Sample(sampler1, input.tex1);

	float4 result = colorTex * input.color;
	result.a = alphaTex.a * input.color.a;

	return ApplyFog(result, input.viewDepth);

#elif defined(TERRAIN_SHADOW)

    float3 shadow = txDiffuse0.Sample(sampler0, input.tex0).rgb;

#   if defined(TERRAIN_SHADOW_CHR)
        shadow *= txDiffuse1.Sample(sampler1, input.tex1).rgb;
#   endif

    return float4(shadow, 1.0f);

#elif defined(TERRAIN_MARKED)

	float4 markTex = txDiffuse0.Sample(sampler0, input.tex0);
	float alpha = txDiffuse1.Sample(sampler1, input.tex1).a;
	
	float4 result = markTex * input.color;
	result.a = textureFactor.a * alpha * input.color.a;
	
	return result;

#else

	return float4(1,0,1,1);

#endif
}
