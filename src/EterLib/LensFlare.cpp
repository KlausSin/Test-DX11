///////////////////////////////////////////////////////////////////////  
//	CLensFlare Class
//
//	(c) 2003 IDV, Inc.
//
//	*** INTERACTIVE DATA VISUALIZATION (IDV) PROPRIETARY INFORMATION ***
//
//	This software is supplied under the terms of a license agreement or
//	nondisclosure agreement with Interactive Data Visualization and may
//	not be copied or disclosed except in accordance with the terms of
//	that agreement.
//
//      Copyright (c) 2001-2003 IDV, Inc.
//      All Rights Reserved.
//
//		IDV, Inc.
//		1233 Washington St. Suite 610
//		Columbia, SC 29201
//		Voice: (803) 799-1699
//		Fax:   (803) 931-0320
//		Web:   http://www.idvinc.com
//


///////////////////////////////////////////////////////////////////////  
//	Preprocessor
#include "StdAfx.h"
#include "LensFlare.h"
#include "Camera.h"
#include "StateManager.h"
#include "ResourceManager.h"

#include <math.h>
using namespace std;

///////////////////////////////////////////////////////////////////////  
//	Variables

static string g_strFiles[] = 
{
	"flare2.dds",
	"flare1.dds",
	"flare2.dds",
	"flare1.dds",
	"flare6.dds",
	"flare4.dds",
	"flare2.dds",
	"flare3.dds",
	""
};
static float g_fPosition[] =
{
	-0.55f,
	-0.5f,
	-0.45f,
	0.2f,
	0.3f,
	0.95f,
	0.9f,
	1.0f
};
static float g_fWidth[] =
{
	20.0f,
	32.0f,
	20.0f,
	32.0f,
	100.0f,
	32.0f,
	20.0f,
	250.0f
};

static float g_afColors[ ][4] =
{
    { 1.0f, 1.0f, 0.0f, 1.0f },
    { 1.0f, 1.0f, 1.0f, 1.0f },
    { 0.0f, 1.0f, 0.0f, 0.8f },
	{ 0.3f, 0.5f, 1.0f, 0.9f },
	{ 0.3f, 0.5f, 1.0f, 0.6f },
	{ 1.0f, 0.6f, 0.9f, 0.4f },
	{ 1.0f, 0.0f, 0.0f, 0.5f },
	{ 1.0f, 0.6f, 0.3f, 0.4f }
};


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::CLensFlare

CLensFlare::CLensFlare() :
    m_fSunSize(0),
    m_fBeforeBright(0.0f),
    m_fAfterBright(0.0f),
    m_bFlareVisible(false),
    m_bDrawFlare(true),
    m_bDrawBrightScreen(true),
	m_bEnabled(true),
	m_bShowMainFlare(true),
	m_fMaxBrightness(1.0f)
{
    m_pControlPixels = new float[c_nDepthTestDimension * c_nDepthTestDimension];
    m_pTestPixels = new float[c_nDepthTestDimension * c_nDepthTestDimension];
	m_afColor[0] = m_afColor[1] = m_afColor[2] = 1.0f;
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::~CLensFlare

CLensFlare::~CLensFlare()
{
    delete[] m_pControlPixels;
    delete[] m_pTestPixels;
}

///////////////////////////////////////////////////////////////////////  
//	CLensFlare::Interpolate

float CLensFlare::Interpolate(float fStart, float fEnd, float fPercent)
{
	return fStart + (fEnd - fStart) * fPercent;
}

///////////////////////////////////////////////////////////////////////  
//	CLensFlare::DrawBeforeFlare

void CLensFlare::Compute(const XMFLOAT3& c_rv3LightDirection)
{
	float afSunPos[3];

	XMFLOAT3 v3Target = CCameraManager::Instance().GetCurrentCamera()->GetTarget();
	
	afSunPos[0]	= v3Target.x - c_rv3LightDirection.x * 99999999.0f;
	afSunPos[1]	= v3Target.y - c_rv3LightDirection.y * 99999999.0f;
	afSunPos[2]	= v3Target.z - c_rv3LightDirection.z * 99999999.0f;
	
	float fX, fY;
	ProjectPosition(afSunPos[0], afSunPos[1], afSunPos[2], &fX, &fY);
	
	// set flare location
	SetFlareLocation(fX, fY);
	
	// determine visibility
	float fSunVectorMagnitude = sqrtf(afSunPos[0] * afSunPos[0] +
		afSunPos[1] * afSunPos[1] +
		afSunPos[2] * afSunPos[2]);
	float afSunVector[3];
	afSunVector[0] = -afSunPos[0] / fSunVectorMagnitude;
	afSunVector[1] = -afSunPos[1] / fSunVectorMagnitude;
	afSunVector[2] = -afSunPos[2] / fSunVectorMagnitude;
	
	float afCameraDirection[3];
	afCameraDirection[0] = ms_matView._13;
	afCameraDirection[1] = ms_matView._23;
	afCameraDirection[2] = ms_matView._33;
	

	float fDotProduct = 
		(afSunVector[0] * afCameraDirection[0]) +
		(afSunVector[1] * afCameraDirection[1]) +
		(afSunVector[2] * afCameraDirection[2]);
	
	if (acosf(fDotProduct) < 0.5f * XM_PI)
		SetVisible(true);
	else
		SetVisible(false);
	
	// set flare brightness
	fX /= ms_Viewport.Width;
	fY /= ms_Viewport.Height;
	
	float fDistance = sqrtf(((0.5f - fX) * (0.5f - fX)) + ((0.5f - fY) * (0.5f - fY)));
	float fBeforeBright = Interpolate(0.0f, c_fHalfMaxBright, 1.0f - (fDistance * c_fDistanceScale));
	float fAfterBright = Interpolate(0.0f, 1.0f, 1.0f - (fDistance * c_fDistanceScale));
	
	SetBrightnesses(fBeforeBright, fAfterBright);
}

///////////////////////////////////////////////////////////////////////  
//	CLensFlare::DrawBeforeFlare

void CLensFlare::DrawBeforeFlare()
{
	if (!m_bFlareVisible || !m_bEnabled || !m_bShowMainFlare)
		return;

	if (m_SunFlareImageInstance.IsEmpty())
		return;

	STATEMANAGER.GetTransform().Push();
	STATEMANAGER.GetDepthStencil().Push();
	STATEMANAGER.GetRaster().Push();
	STATEMANAGER.GetBlend().Push();

	XMFLOAT4X4 matProj;
	XMStoreFloat4x4(&matProj, XMMatrixOrthographicOffCenterRH(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f));

	XMFLOAT4X4 matWorld;
	XMStoreFloat4x4(&matWorld, XMMatrixTranslation(m_afFlarePos[0], m_afFlarePos[1], 0.0f));

	STATEMANAGER.GetTransform().SetProjection(matProj);
	STATEMANAGER.GetTransform().SetView(ms_matIdentity);
	STATEMANAGER.GetTransform().SetWorld(matWorld);

	_mgr->GetCbMgr()->SetLightingEnable(false);
	_mgr->GetCbMgr()->SetAlphaTestEnable(false);

	STATEMANAGER.GetDepthStencil().SetDepthEnable(false);
	STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(false);
	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	float fAspectRatio = ms_Viewport.Width / float(ms_Viewport.Height);
	float fHeight = m_fSunSize * fAspectRatio;
	XMFLOAT4 color(1.0f, 1.0f, 1.0f, 1.0f);

	SVertex vertices[4];
	vertices[0].x = -m_fSunSize;
	vertices[0].y = -fHeight;
	vertices[0].z = 0.0f;
	vertices[0].color = ColorToUint(color);
	vertices[0].u = 0.0f;
	vertices[0].v = 0.0f;

	vertices[1].x = -m_fSunSize;
	vertices[1].y = fHeight;
	vertices[1].z = 0.0f;
	vertices[1].color = ColorToUint(color);
	vertices[1].u = 0.0f;
	vertices[1].v = 1.0f;

	vertices[2].x = m_fSunSize;
	vertices[2].y = -fHeight;
	vertices[2].z = 0.0f;
	vertices[2].color = ColorToUint(color);
	vertices[2].u = 1.0f;
	vertices[2].v = 0.0f;

	vertices[3].x = m_fSunSize;
	vertices[3].y = fHeight;
	vertices[3].z = 0.0f;
	vertices[3].color = ColorToUint(color);
	vertices[3].u = 1.0f;
	vertices[3].v = 1.0f;

	STATEMANAGER.SetTexture(0, m_SunFlareImageInstance.GetTexturePointer()->GetSRV());
	STATEMANAGER.SetTexture(1, NULL);

	_mgr->SetShader(VF_PDT, BLEND_MODULATE);
	STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TVertex), vertices);

	STATEMANAGER.GetBlend().Restore();
	STATEMANAGER.GetRaster().Restore();
	STATEMANAGER.GetDepthStencil().Restore();
	STATEMANAGER.GetTransform().Restore();
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::DrawAfterFlare

void CLensFlare::DrawAfterFlare()
{
	if (m_bEnabled && m_fAfterBright != 0.0f && m_bDrawBrightScreen)
	{
		SetDiffuseColor(m_afColor[0], m_afColor[1], m_afColor[2], m_fAfterBright);
		RenderBar2d(0.0f, 0.0f, 1024.0f, 1024.0f);
	}
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::SetMainFlare

void CLensFlare::SetMainFlare(string strSunFile, float fSunSize)
{
	if (m_bEnabled && m_bShowMainFlare)
	{
		m_fSunSize = fSunSize;
		CResource * pResource = CResourceManager::Instance().GetResourcePointer(strSunFile.c_str()); 

		if (!pResource->IsType(CGraphicImage::Type()))
			assert(false);
		
		m_SunFlareImageInstance.SetImagePointer(static_cast<CGraphicImage *> (pResource));
	}
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::DrawFlare

void CLensFlare::DrawFlare()
{
	if (m_bEnabled && m_bFlareVisible && m_bDrawFlare && m_fAfterBright != 0.0f)
	{
		STATEMANAGER.GetTransform().Push();
		STATEMANAGER.GetDepthStencil().Push();
		STATEMANAGER.GetRaster().Push();
		STATEMANAGER.GetBlend().Push();

		_mgr->GetCbMgr()->SetLightingEnable(false);
		_mgr->GetCbMgr()->SetAlphaTestEnable(false);

		STATEMANAGER.GetDepthStencil().SetDepthEnable(false);
		STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);
		STATEMANAGER.GetBlend().SetBlendEnable(true);

		XMFLOAT4X4 matProj;
		XMStoreFloat4x4(&matProj, XMMatrixOrthographicOffCenterRH(0.0f, ms_Viewport.Width, ms_Viewport.Height, 0.0f, -1.0f, 1.0f));

		STATEMANAGER.GetTransform().SetProjection(matProj);
		STATEMANAGER.GetTransform().SetView(ms_matIdentity);
		STATEMANAGER.GetTransform().SetWorld(ms_matIdentity);

		DrawAfterFlare();

		m_cFlare.Draw(
			m_fAfterBright,
			ms_Viewport.Width,
			ms_Viewport.Height,
			static_cast<int>(m_afFlareWinPos[0]),
			static_cast<int>(m_afFlareWinPos[1]));

		STATEMANAGER.GetBlend().Restore();
		STATEMANAGER.GetRaster().Restore();
		STATEMANAGER.GetDepthStencil().Restore();
		STATEMANAGER.GetTransform().Restore();
	}
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::CharacterizeFlare
void CLensFlare::CharacterizeFlare(bool bEnabled, bool bShowMainFlare, float fMaxBrightness, const XMFLOAT4 & c_rColor)
{
	m_bEnabled = bEnabled;
	m_bShowMainFlare = bShowMainFlare;
	m_fMaxBrightness = fMaxBrightness;

	m_afColor[0] = c_rColor.x;
	m_afColor[1] = c_rColor.y;
	m_afColor[2] = c_rColor.z;
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::Initialize
void CLensFlare::Initialize(std::string strPath)
{
	if (m_bEnabled)
		m_cFlare.Init(strPath);
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::SetFlareLocation
void CLensFlare::SetFlareLocation(double dX, double dY)
{
	if (m_bEnabled)
	{
		m_afFlareWinPos[0] = float(dX);
		m_afFlareWinPos[1] = float(dY);

		m_afFlarePos[0] = float(dX) / ms_Viewport.Width;
		m_afFlarePos[1] = float(dY) / ms_Viewport.Height;
	}
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::SetBrightnesses

void CLensFlare::SetBrightnesses(float fBeforeBright, float fAfterBright)
{
	if (m_bEnabled)
	{
	    m_fBeforeBright = fBeforeBright;
	    m_fAfterBright = fAfterBright;

		ClampBrightness();
	}
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::ReadControlPixels

void CLensFlare::ReadControlPixels()
{
	if (m_bEnabled)
		ReadDepthPixels(m_pControlPixels);
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::AdjustBrightness

void CLensFlare::AdjustBrightness()
{
	if (m_bEnabled)
	{
		ReadDepthPixels(m_pTestPixels);

		int nDifferent = 0;

		for (int i = 0; i < c_nDepthTestDimension * c_nDepthTestDimension; ++i)
			if (m_pTestPixels[i] != m_pControlPixels[i])
				++nDifferent;

		float fAdjust = (static_cast<float>(nDifferent) / (c_nDepthTestDimension * c_nDepthTestDimension));
		fAdjust = sqrtf(fAdjust) * 0.85f;
		m_fAfterBright *= 1.0f - fAdjust;
	}
}


///////////////////////////////////////////////////////////////////////  
//	CLensFlare::ReadDepthPixels

void CLensFlare::ReadDepthPixels(float * /*pPixels*/)
{
	/*
	LPDIRECT3DSURFACE9 lpSurface;
	if (FAILED(ms_lpd3dDevice->GetDepthStencilSurface(&lpSurface)))
		assert(false);

	D3DLOCKED_RECT rect;
	lpSurface->LockRect(&rect, NULL, D3DLOCK_READONLY | D3DLOCK_NO_DIRTY_UPDATE);

	lpSurface->UnlockRect();
	*/
	/*
	glReadPixels(GLint(m_afFlareWinPos[0] - c_nDepthTestDimension / 2),
				 GLint(m_afFlareWinPos[1] - c_nDepthTestDimension / 2),
				 c_nDepthTestDimension, c_nDepthTestDimension,
				 GL_DEPTH_COMPONENT, GL_FLOAT, pPixels);
	*/
}

///////////////////////////////////////////////////////////////////////  
//	CLensFlare::ClampBrightness

void CLensFlare::ClampBrightness()
{
	// before
    if (m_fBeforeBright < 0.0f)
        m_fBeforeBright = 0.0f;
    else if (m_fBeforeBright > 1.0f)
        m_fBeforeBright = 1.0f;

	m_fBeforeBright *= m_fMaxBrightness;

    if (m_fAfterBright < 0.0f)
        m_fAfterBright = 0.0f;
    else if (m_fAfterBright > 1.0f)
        m_fAfterBright = 1.0f;
	
	m_fAfterBright *= m_fMaxBrightness;
}

///////////////////////////////////////////////////////////////////////  
//	CFlare implementation
///////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////  
//	CFlare::CFlare

CFlare::CFlare()
{
}


///////////////////////////////////////////////////////////////////////  
//	CFlare::~CFlare

CFlare::~CFlare()
{
}


///////////////////////////////////////////////////////////////////////  
//	CFlare::Init

void CFlare::Init(std::string strPath)
{
	int i = 0;

	while (g_strFiles[i] != "")
	{
		CResource * pResource = CResourceManager::Instance().GetResourcePointer((strPath + "/" + string(g_strFiles[i])).c_str());
		
		if (!pResource->IsType(CGraphicImage::Type()))
			assert(false);

		SFlarePiece * pPiece = new SFlarePiece;

		pPiece->m_imageInstance.SetImagePointer(static_cast<CGraphicImage *> (pResource));
		pPiece->m_fPosition = g_fPosition[i];
		pPiece->m_fWidth = g_fWidth[i];
		pPiece->m_pColor = g_afColors[i];

		m_vFlares.push_back(pPiece);
		i++;
	}
}


///////////////////////////////////////////////////////////////////////  
//	CFlare::Draw
void CFlare::Draw(float fBrightScale, int nWidth, int nHeight, int nX, int nY)
{
	STATEMANAGER.GetBlend().Push();
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_ONE);

	float fDX = float(nX) - float(nWidth) / 2.0f;
	float fDY = float(nY) - float(nHeight) / 2.0f;

	STATEMANAGER.SetTexture(1, NULL);
	_mgr->SetShader(VF_PDT, BLEND_MODULATE);

	for (unsigned int i = 0; i < m_vFlares.size(); i++)
	{
		float fCenterX = float(nX) - (m_vFlares[i]->m_fPosition + 1.0f) * fDX;
		float fCenterY = float(nY) - (m_vFlares[i]->m_fPosition + 1.0f) * fDY;
		float fW = m_vFlares[i]->m_fWidth;
		
		XMFLOAT4 d3dColor(m_vFlares[i]->m_pColor[0] * fBrightScale,
						   m_vFlares[i]->m_pColor[1] * fBrightScale,
						   m_vFlares[i]->m_pColor[2] * fBrightScale,
						   m_vFlares[i]->m_pColor[3] * fBrightScale);

		STATEMANAGER.SetTexture(0, m_vFlares[i]->m_imageInstance.GetTexturePointer()->GetSRV());

		TVertex vertices[4];
		
		vertices[0].u = 0.0f;
		vertices[0].v = 0.0f;
		vertices[0].x = fCenterX - fW;
		vertices[0].y = fCenterY - fW;
		vertices[0].z = 0.0f;
		vertices[0].color = ColorToUint(d3dColor);

		vertices[1].u = 0.0f;
		vertices[1].v = 1.0f;
		vertices[1].x = fCenterX - fW;
		vertices[1].y = fCenterY + fW;
		vertices[1].z = 0.0f;
		vertices[1].color = ColorToUint(d3dColor);

		vertices[2].u = 1.0f;
		vertices[2].v = 0.0f;
		vertices[2].x = fCenterX + fW;
		vertices[2].y = fCenterY - fW;
		vertices[2].z = 0.0f;
		vertices[2].color = ColorToUint(d3dColor);

		vertices[3].u = 1.0f;
		vertices[3].v = 1.0f;
		vertices[3].x = fCenterX + fW;
		vertices[3].y = fCenterY + fW;
		vertices[3].z = 0.0f;
		vertices[3].color = ColorToUint(d3dColor);

		STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 2, sizeof(TVertex), vertices);
	}

	STATEMANAGER.GetBlend().Restore();
}
