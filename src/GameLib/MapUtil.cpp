#include "StdAfx.h"
#include "MapUtil.h"
#include "PackLib/PackManager.h"
#include <EterBase/FileLoaderJson.h>

void Environment_Init(SEnvironmentData& envData)
{
	for (int i = 0; i < ENV_DIRLIGHT_NUM; ++i)
	{
		envData.bDirLightsEnable[i] = false;
		envData.DirLights[i].Type = D3DLIGHT_DIRECTIONAL11;
		envData.DirLights[i].Direction = XMFLOAT3(0.5f, 0.5f, -0.5f);
		envData.DirLights[i].Position = XMFLOAT3(0.0f, 0.0f, 0.0f);
		envData.DirLights[i].Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		envData.DirLights[i].Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
		envData.DirLights[i].Ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
		envData.DirLights[i].Range = 0.0f; // Used by Point Light & Spot Light
		envData.DirLights[i].Falloff = 1.0f; // Used by Spot Light
		envData.DirLights[i].Theta = 0.0f; // Used by Spot Light
		envData.DirLights[i].Phi = 0.0f; // Used by Spot Light
		envData.DirLights[i].Attenuation0 = 0.0f;
		envData.DirLights[i].Attenuation1 = 1.0f;
		envData.DirLights[i].Attenuation2 = 0.0f;
	}

	envData.Material.Diffuse = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	envData.Material.Ambient = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	envData.Material.Emissive = XMFLOAT4(0.8f, 0.8f, 0.8f, 1.0f);
	envData.Material.Specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	envData.Material.Power = 0.0f;

	// MR-14: Fog update by Alaric
	envData.bFogEnable = TRUE;
	envData.bDensityFog = TRUE;
	envData.bFogLevel = 0;
	// MR-14: -- END OF -- Fog update by Alaric
	envData.m_fFogNearDistance = 25600.0f * 0.5f;
	envData.m_fFogFarDistance = 25600.0f * 0.7f;
	envData.FogColor = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);

	envData.bFilteringEnable = FALSE;
	envData.FilteringColor = XMFLOAT4(0.3f, 0.1f, 0.1f, 0.0f);
	envData.byFilteringAlphaSrc = D3D11_BLEND_ONE;
	envData.byFilteringAlphaDest = D3D11_BLEND_ONE;

	envData.fWindStrength = 0.2f;
	envData.fWindRandom = 0.0f;

	envData.v3SkyBoxScale = XMFLOAT3(3500.0f, 3500.0f, 3500.0f);
	envData.bySkyBoxGradientLevelUpper = 0;
	envData.bySkyBoxGradientLevelLower = 0;
	envData.bSkyBoxTextureRenderMode = FALSE;

	envData.v2CloudScale = XMFLOAT2(200000.0f, 200000.0f);
	envData.fCloudHeight = 30000.0f;
	envData.v2CloudTextureScale = XMFLOAT2(4.0f, 4.0f);
	envData.v2CloudSpeed = XMFLOAT2(0.001f, 0.001f);
	envData.strCloudTextureFileName = "";
	envData.CloudGradientColor.m_FirstColor = .0f;
	envData.CloudGradientColor.m_SecondColor = .0f;

	envData.SkyBoxGradientColorVector.clear();

	envData.bLensFlareEnable = FALSE;
	envData.LensFlareBrightnessColor = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	envData.fLensFlareMaxBrightness = 1.0f;

	envData.bMainFlareEnable = FALSE;
	envData.strMainFlareTextureFileName = "";
	envData.fMainFlareSize = 0.2f;

	envData.bReserve = FALSE;
}

bool Environment_Load(SEnvironmentData& envData, const char* envFileName)
{
    CMemoryJsonFileLoader json;

    if (!json.Load(envFileName))
        return false;

    json.GetTokenBoolean("reserved", &envData.bReserve);

    if (json.SetChildNode("directionallight"))
    {
        XMFLOAT3 v3Dir{};
        json.GetTokenVector3("direction", &v3Dir);

        if (json.SetChildNode("background"))
        {
            envData.DirLights[ENV_DIRLIGHT_BACKGROUND].Direction = v3Dir;

            json.GetTokenBoolean("enable", &envData.bDirLightsEnable[ENV_DIRLIGHT_BACKGROUND]);
            json.GetTokenColor("diffuse", &envData.DirLights[ENV_DIRLIGHT_BACKGROUND].Diffuse);
            json.GetTokenColor("ambient", &envData.DirLights[ENV_DIRLIGHT_BACKGROUND].Ambient);

            json.SetParentNode();
        }

        if (json.SetChildNode("character"))
        {
            envData.DirLights[ENV_DIRLIGHT_CHARACTER].Direction = v3Dir;

            json.GetTokenBoolean("enable", &envData.bDirLightsEnable[ENV_DIRLIGHT_CHARACTER]);
            json.GetTokenColor("diffuse", &envData.DirLights[ENV_DIRLIGHT_CHARACTER].Diffuse);
            json.GetTokenColor("ambient", &envData.DirLights[ENV_DIRLIGHT_CHARACTER].Ambient);

            json.SetParentNode();
        }

        json.SetParentNode();
    }

    if (json.SetChildNode("material"))
    {
        json.GetTokenColor("diffuse", &envData.Material.Diffuse);
        json.GetTokenColor("ambient", &envData.Material.Ambient);
        json.GetTokenColor("emissive", &envData.Material.Emissive);

        json.SetParentNode();
    }

    if (json.SetChildNode("fog"))
    {
        if (json.GetTokenByte("foglevel", &envData.bFogLevel))
        {
            envData.bDensityFog = true;
        }
        else
        {
            envData.bDensityFog = false;

            json.GetTokenBoolean("enable", &envData.bFogEnable);
            json.GetTokenFloat("neardistance", &envData.m_fFogNearDistance);
            json.GetTokenFloat("fardistance", &envData.m_fFogFarDistance);
        }

        json.GetTokenColor("color", &envData.FogColor);

        json.SetParentNode();
    }

    if (json.SetChildNode("filter"))
    {
        json.GetTokenBoolean("enable", &envData.bFilteringEnable);
        json.GetTokenColor("color", &envData.FilteringColor);
        json.GetTokenByte("alphasrc", &envData.byFilteringAlphaSrc);
        json.GetTokenByte("alphadest", &envData.byFilteringAlphaDest);

        json.SetParentNode();
    }

    if (json.SetChildNode("skybox"))
    {
        json.GetTokenBoolean("btexturerendermode", &envData.bSkyBoxTextureRenderMode);
        json.GetTokenVector3("scale", &envData.v3SkyBoxScale);

        json.GetTokenByte("gradientlevelupper", &envData.bySkyBoxGradientLevelUpper);
        json.GetTokenByte("gradientlevellower", &envData.bySkyBoxGradientLevelLower);

        json.GetTokenString("frontfacefilename", &envData.strSkyBoxFaceFileName[0]);
        json.GetTokenString("backfacefilename", &envData.strSkyBoxFaceFileName[1]);
        json.GetTokenString("leftfacefilename", &envData.strSkyBoxFaceFileName[2]);
        json.GetTokenString("rightfacefilename", &envData.strSkyBoxFaceFileName[3]);
        json.GetTokenString("topfacefilename", &envData.strSkyBoxFaceFileName[4]);
        json.GetTokenString("bottomfacefilename", &envData.strSkyBoxFaceFileName[5]);

        json.GetTokenVector2("cloudscale", &envData.v2CloudScale);
        json.GetTokenFloat("cloudheight", &envData.fCloudHeight);
        json.GetTokenVector2("cloudtexturescale", &envData.v2CloudTextureScale);
        json.GetTokenVector2("cloudspeed", &envData.v2CloudSpeed);

        json.GetTokenString("cloudtexturefilename", &envData.strCloudTextureFileName);

        std::vector<float> cloudColor;

        if (json.GetTokenArray("cloudcolor", &cloudColor) && cloudColor.size() >= 8)
        {
            envData.CloudGradientColor.m_FirstColor.r = cloudColor[0];
            envData.CloudGradientColor.m_FirstColor.g = cloudColor[1];
            envData.CloudGradientColor.m_FirstColor.b = cloudColor[2];
            envData.CloudGradientColor.m_FirstColor.a = cloudColor[3];

            envData.CloudGradientColor.m_SecondColor.r = cloudColor[4];
            envData.CloudGradientColor.m_SecondColor.g = cloudColor[5];
            envData.CloudGradientColor.m_SecondColor.b = cloudColor[6];
            envData.CloudGradientColor.m_SecondColor.a = cloudColor[7];
        }

        auto gradient = json.GetArray("gradient");

        envData.SkyBoxGradientColorVector.clear();

        for (auto g : gradient)
        {
            simdjson::dom::array arr;

            if (g.get(arr))
                continue;

            std::vector<float> vals;

            for (auto v : arr)
                vals.push_back(float(double(v)));

            if (vals.size() < 8)
                continue;

            TGradientColor color{};

            color.m_FirstColor.r = vals[0];
            color.m_FirstColor.g = vals[1];
            color.m_FirstColor.b = vals[2];
            color.m_FirstColor.a = vals[3];

            color.m_SecondColor.r = vals[4];
            color.m_SecondColor.g = vals[5];
            color.m_SecondColor.b = vals[6];
            color.m_SecondColor.a = vals[7];

            envData.SkyBoxGradientColorVector.push_back(color);
        }

        json.SetParentNode();
    }

    if (json.SetChildNode("lensflare"))
    {
        json.GetTokenBoolean("enable", &envData.bLensFlareEnable);
        json.GetTokenColor("brightnesscolor", &envData.LensFlareBrightnessColor);
        json.GetTokenFloat("maxbrightness", &envData.fLensFlareMaxBrightness);
        json.GetTokenBoolean("mainflareenable", &envData.bMainFlareEnable);
        json.GetTokenString("mainflaretexturefilename", &envData.strMainFlareTextureFileName);
        json.GetTokenFloat("mainflaresize", &envData.fMainFlareSize);

        json.SetParentNode();
    }

    return true;
}

void GetInterpolatedPosition(float curPositionRate, TPixelPosition * PixelPosition)
{
}

float GetLinearInterpolation(float begin, float end, float curRate)
{
	return (end - begin) * curRate + begin;
}

void PixelPositionToAttributeCellPosition(TPixelPosition PixelPosition, TCellPosition * pAttrCellPosition)
{
	pAttrCellPosition->x = PixelPosition.x / c_Section_xAttributeCellSize;
	pAttrCellPosition->y = PixelPosition.y / c_Section_yAttributeCellSize;
}

void AttributeCellPositionToPixelPosition(TCellPosition AttrCellPosition, TPixelPosition * pPixelPosition)
{
	pPixelPosition->x = AttrCellPosition.x * c_Section_xAttributeCellSize;
	pPixelPosition->y = AttrCellPosition.y * c_Section_yAttributeCellSize;
}

float GetPixelPositionDistance(const TPixelPosition & c_rsrcPosition, const TPixelPosition & c_rdstPosition)
{
	int idx = c_rsrcPosition.x - c_rdstPosition.x;
	int idy = c_rsrcPosition.y - c_rdstPosition.y;

	return sqrtf(float(idx*idx + idy*idy));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
CEaseOutInterpolation::CEaseOutInterpolation()
{
	Initialize();
}

CEaseOutInterpolation::~CEaseOutInterpolation()
{
}

void CEaseOutInterpolation::Initialize()
{
	m_fRemainingTime = 0.0f;
	m_fValue = 0.0f;
	m_fSpeed = 0.0f;
	m_fAcceleration = 0.0f;
	m_fStartValue = 0.0f;
	m_fLastValue = 0.0f;
}


BOOL CEaseOutInterpolation::Setup(float fStart, float fEnd, float fTime)
{
	//for safety 
	if( fabs(fTime) < FLT_EPSILON )
	{
		fTime = 0.01f;
	}

	m_fValue = fStart;
	m_fStartValue = fStart;
	m_fLastValue = fStart;

	m_fSpeed = (2.0f * (fEnd - fStart)) / fTime;
	m_fAcceleration = 2.0f * (fEnd - fStart) / (fTime * fTime) - 2.0f * m_fSpeed / fTime;
	m_fRemainingTime = fTime;

	return TRUE;
}

void CEaseOutInterpolation::Interpolate(float fElapsedTime)
{
	m_fLastValue = m_fValue;

	m_fRemainingTime -= fElapsedTime;
	m_fSpeed += m_fAcceleration * fElapsedTime;
	m_fValue += m_fSpeed * fElapsedTime;

	if (!isPlaying())
	{
		m_fValue = 0.0f;
		m_fLastValue = 0.0f;
	}
}

BOOL CEaseOutInterpolation::isPlaying()
{
	return m_fRemainingTime > 0.0f;
}

float CEaseOutInterpolation::GetValue()
{
	return m_fValue;
}

float CEaseOutInterpolation::GetChangingValue()
{
	return m_fValue - m_fLastValue;
}
