#include "common.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;

#ifdef IS_SKINNED
    float4 weights : BLENDWEIGHT;
    uint4 indices : BLENDINDICES;
#endif

    float2 tex0 : TEXCOORD0;
#ifdef HAS_TEX2
    float2 tex1 : TEXCOORD1;
#endif
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
    float viewDepth : TEXCOORD2;
    float2 shadowUV : TEXCOORD3;
};

#ifdef IS_SKINNED
static void SkinVertex(VS_INPUT input, out float4 skinnedPos, out float3 skinnedNormal)
{
    float4 p = float4(input.pos, 1.0f);
    float3 n = input.normal;

    skinnedPos =
        mul(p, bonePalette[input.indices.x]) * input.weights.x +
        mul(p, bonePalette[input.indices.y]) * input.weights.y +
        mul(p, bonePalette[input.indices.z]) * input.weights.z +
        mul(p, bonePalette[input.indices.w]) * input.weights.w;

    skinnedNormal =
        mul(n, (float3x3)bonePalette[input.indices.x]) * input.weights.x +
        mul(n, (float3x3)bonePalette[input.indices.y]) * input.weights.y +
        mul(n, (float3x3)bonePalette[input.indices.z]) * input.weights.z +
        mul(n, (float3x3)bonePalette[input.indices.w]) * input.weights.w;
}
#endif

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

#ifdef IS_SKINNED
    float4 localPos;
    float3 localNormal;
    SkinVertex(input, localPos, localNormal);
    localNormal = normalize(localNormal);
#else
    float4 localPos = float4(input.pos, 1.0f);
    float3 localNormal = input.normal;
#endif

    float4 worldPos = mul(localPos, frame.matWorld);
    float4 viewPos = mul(worldPos, frame.matView);

    output.pos = mul(viewPos, frame.matProj);
    output.viewDepth = abs(viewPos.z);
    output.tex0 = input.tex0;

    float4 shadowCoord = mul(worldPos, texTransform.tex1);
    output.shadowUV = shadowCoord.xy;

    float3 worldNormal = normalize(mul(localNormal, (float3x3) frame.matWorld));
    output.color = float4(ApplyEntityLights(worldPos.xyz, worldNormal), material.diffuse.a);

#if defined(MESH_SPECULAR)
    float3 viewNormal = normalize(mul(worldNormal, (float3x3)frame.matView));
    float3 viewDir = normalize(-viewPos.xyz);
    float3 reflVec = reflect(-viewDir, viewNormal);

    float4 tc = mul(float4(reflVec, 1.0f), texTransform.tex1);
    output.tex1 = tc.xy;
#else
    output.tex1 = float2(0.0f, 0.0f);
#endif

#ifdef HAS_TEX2
#ifndef MESH_SPECULAR
        output.tex1 = input.tex1;
#endif
#endif

    return output;
}