#include "common.hlsli"

Texture2D tex0 : register(t0);
Texture2D tex1 : register(t1);
SamplerState samp0 : register(s0);
SamplerState samp1 : register(s1);

struct PS_IN
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float fog : TEXCOORD2;
};

static const float gAlphaRef = 84.0f / 255.0f;

float4 main(PS_IN i) : SV_Target
{
    float4 texColor = tex0.Sample(samp0, i.uv0);

    float3 light = saturate(gSpeedTreeLightAmbient.rgb + gSpeedTreeLightDiffuse.rgb);

    float4 baseColor = texColor * i.color;
    baseColor.rgb *= light;

    clip(baseColor.a - gAlphaRef);

    return baseColor;
}
