#include "common.hlsli"

struct VS_IN
{
    float3 pos : POSITION;
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float2 uv1 : TEXCOORD1;
    float fog : TEXCOORD2;
};

VS_OUT main(VS_IN v)
{
    VS_OUT o;
    float4 worldPos = float4(v.pos + gTreePos.xyz, 1.0f);
    o.pos = mul(worldPos, gSpeedTreeCompound);
    o.color = v.color;
    o.uv0 = v.uv0;
    o.uv1 = v.uv1;
    float dist = mul(worldPos, frame.matView).z;
    o.fog = saturate((gSpeedTreeFog.y - dist) * gSpeedTreeFog.z);
    return o;
}
