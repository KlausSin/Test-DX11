#include "common.hlsli"

struct VS_IN
{
    float3 pos : POSITION;
    float4 color : COLOR0;
    float2 uv0 : TEXCOORD0;
    float4 leafData : TEXCOORD2;
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
    uint tableIndex = (uint) (v.leafData.z + 0.5f);
    float side = v.leafData.x;

    if (tableIndex < 1024)
    {
        float3 o = gLeafTable[tableIndex].xyz;

        if (side > 0.5f)
            o = float3(-o.y, o.x, o.z);

        v.pos += o * v.leafData.w;
    }

    VS_OUT o;
    float4 worldPos = float4(v.pos + gTreePos.xyz, 1.0f);
    o.pos = mul(worldPos, gSpeedTreeCompound);
    o.color = v.color;
    o.uv0 = v.uv0;
    o.uv1 = float2(0.0f, 0.0f);
    float dist = mul(worldPos, matView).z;
    o.fog = saturate((gSpeedTreeFog.y - dist) * gSpeedTreeFog.z);
    return o;
}
