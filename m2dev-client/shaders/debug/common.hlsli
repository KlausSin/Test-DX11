struct cbPerFrame
{
	row_major float4x4 matWorld;
	row_major float4x4 matView;
	row_major float4x4 matProj;
};

struct cbTexTransform
{
    row_major float4x4 tex0;
    row_major float4x4 tex1;
    row_major float4x4 tex2;
    row_major float4x4 tex3;
};

cbuffer cbMatrix : register(b0)
{
    cbPerFrame frame;
    cbTexTransform texTransform;
}

cbuffer cbMaterial : register(b1)
{
	float4 textureFactor;
	int useTexture0; 
	int useTexture1;
	int alphaTestEnable; 
	int alphaRef;
};

cbuffer cbFog : register(b4)
{
    float4 fogColor;
    float  fogStart;
    float  fogEnd;
    int    fogEnable;
    int    fogPad;
};

cbuffer CBScreenSize : register(b5)
{
    float screenWidth;
    float screenHeight;
    float padScreen0;
    float padScreen1;
};

cbuffer CBSpeedTree : register(b7)
{
    row_major float4x4 gSpeedTreeCompound;
    float4 gTreePos;
    float4 gSpeedTreeFog;
    float4 gSpeedTreeLightDir;
    float4 gSpeedTreeLightAmbient;
    float4 gSpeedTreeLightDiffuse;
    float4 gSpeedTreeMaterialDiffuse;
    float4 gSpeedTreeMaterialAmbient;
    float4 gLeafLightingAdjustment;
    float4 gLeafTable[1024];
};

float4 ResolveArg(int arg, float4 tex, float4 diffuse)
{
	if (arg == 3) return textureFactor;
	if (arg == 2) return tex;
	return diffuse;
}

cbuffer GrannyBonePalette : register(b6)
{
    row_major float4x4 bonePalette[256];
};
cbuffer TerrainLayerCB : register(b8)
{
    float4 gBrush;
    float4 gFlags;
    float4 gEye;
    float4 gLayerTiling[8];
    float4 gLayerStrength[8];
    float4 gLayerEnabled[8];
};


#define MAX_ENTITY_LIGHTS 32

struct EntityLight
{
    float4 position;
    float4 direction;
    float4 diffuse;
    float4 ambient;
    float4 attenuation;
    float4 spot;
    int4 params;
};

struct CBMaterialX
{
    float4 diffuse;
    float4 ambient;
    float4 emissive;
    float4 specular;
    float4 params;
};

cbuffer EntityLightingBuffer : register(b9)
{
    EntityLight g_entityLights[MAX_ENTITY_LIGHTS];
    CBMaterialX material;
    float4 g_entityGlobalAmbient;
    int4 g_entityLightingSettings;
};

float3 ApplyEntityLights(float3 worldPos, float3 normal)
{
    float3 n = normalize(normal);
    float3 result = material.emissive.rgb + material.ambient.rgb * g_entityGlobalAmbient.rgb;

    if (g_entityLightingSettings.y == 0)
        return saturate(material.diffuse.rgb);

    int count = g_entityLightingSettings.x;

    for (int i = 0; i < count; ++i)
    {
        EntityLight l = g_entityLights[i];

        if (l.params.y == 0)
            continue;

        int type = l.params.x;
        float3 lightColor = l.diffuse.rgb * l.diffuse.a;
        float3 ambient = material.ambient.rgb * l.ambient.rgb;

        if (type == 3)
        {
            float3 L = normalize(-l.direction.xyz);
            float ndl = saturate(dot(n, L));
            result += ambient + material.diffuse.rgb * lightColor * ndl;
        }
        else
        {
            float3 toLight = l.position.xyz - worldPos;
            float dist = length(toLight);
            float3 L = toLight / max(dist, 0.0001f);

            float range = max(l.attenuation.w, 0.0001f);
            float att = saturate(1.0f - dist / range);
            att *= att;

            float ndl = saturate(dot(n, L));

            if (type == 2)
            {
                float3 spotDir = normalize(l.direction.xyz);
                float spot = dot(-L, spotDir);
                float cone = saturate((spot - cos(l.spot.y)) / max(cos(l.spot.x) - cos(l.spot.y), 0.0001f));
                att *= pow(cone, l.spot.z);
            }

            result += ambient * att + material.diffuse.rgb * lightColor * ndl * att;
        }
    }

    return saturate(result);
}
