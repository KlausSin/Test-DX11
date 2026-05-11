#include "common.hlsli"

struct VS_INPUT
{
    float3 pos : POSITION;
    float3 normal : NORMAL;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float4 color : COLOR0;
    float2 tex0 : TEXCOORD0;
    float2 tex1 : TEXCOORD1;
    float2 texShadowStatic : TEXCOORD2;
    float2 texShadowCharacter : TEXCOORD3;
    float viewDepth : TEXCOORD4;
};

float4 BuildTerrainLight(float3 normal)
{
    if (lightingEnable == 0)
        return float4(1.0f, 1.0f, 1.0f, 1.0f);

    float3 worldNormal = normalize(mul(normal, (float3x3)frame.matWorld));
    float ndotl = saturate(dot(worldNormal, normalize(-lightDir.xyz)));
    float3 ambient = matAmbient.rgb * lightAmbient.rgb;
    float3 diffuse = matDiffuse.rgb * lightDiffuse.rgb * ndotl;
    float3 emissive = matEmissive.rgb;
    return float4(saturate(ambient + diffuse + emissive), matDiffuse.a);
}

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;

    float4 worldPos = mul(float4(input.pos, 1.0f), frame.matWorld);
    float4 viewPos = mul(worldPos, frame.matView);

    output.pos = mul(viewPos, frame.matProj);
    output.viewDepth = abs(viewPos.z);
    output.color = BuildTerrainLight(input.normal);

    float4 texSource = float4(viewPos.xyz, 1.0f);
    output.tex0 = mul(texSource, texTransform.tex0).xy;
    output.tex1 = mul(texSource, texTransform.tex1).xy;
    output.texShadowStatic = mul(texSource, texTransform.tex2).xy;
    output.texShadowCharacter = mul(texSource, texTransform.tex3).xy;

    return output;
}
