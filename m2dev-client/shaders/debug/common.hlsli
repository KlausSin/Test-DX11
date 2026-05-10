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

cbuffer cbLighting : register(b2)
{
	float4 lightDir; 
	float4 lightDiffuse; 
	float4 lightAmbient;
	float4 matDiffuse; 
	float4 matAmbient; 
	float4 matEmissive;
	int lightingEnable; 
	int pad0; int pad1; int pad2;
	float4 specularColor; // rgb = culoare, a = power
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
	return diffuse;  // DIFFUSE(0) or CURRENT(1)
}

cbuffer GrannyBonePalette : register(b6)
{
    row_major float4x4 bonePalette[256];
};
