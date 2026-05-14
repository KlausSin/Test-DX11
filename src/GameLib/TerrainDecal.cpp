// TerrainDecal.cpp: implementation of the CTerrainDecal class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EterLib/StateManager.h"
#include "PRTerrainLib/StdAfx.h"

#include "TerrainDecal.h"
#include "MapOutdoor.h"
#include "AreaTerrain.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTerrainDecal::CTerrainDecal(CMapOutdoor * pMapOutdoor):m_pMapOutdoor(pMapOutdoor)
{

}

CTerrainDecal::~CTerrainDecal()
{
	CDecal::Clear();
}

void CTerrainDecal::Make(XMFLOAT3 v3Center, XMFLOAT3 v3Normal, XMFLOAT3 v3Tangent, float fWidth, float fHeight, float fDepth)
{
	Clear();

	m_v3Center = v3Center;

	XMStoreFloat3(&v3Normal, XMVector3Normalize(XMLoadFloat3(&v3Normal)));
	XMStoreFloat3(&v3Tangent, XMVector3Normalize(XMLoadFloat3(&v3Tangent)));

	m_v3Normal = v3Normal;

	XMFLOAT3 v3Binormal;
	XMStoreFloat3(&v3Binormal, XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&m_v3Normal), XMLoadFloat3(&v3Tangent))));

	float fd = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_v3Center), XMLoadFloat3(&v3Tangent)));
	m_v4LeftPlane = XMFLOAT4(v3Tangent.x, v3Tangent.y, v3Tangent.z, fWidth * 0.5f - fd);
	m_v4RightPlane = XMFLOAT4(-v3Tangent.x, -v3Tangent.y, -v3Tangent.z, fWidth * 0.5f + fd);

	fd = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_v3Center), XMLoadFloat3(&v3Binormal)));
	m_v4BottomPlane = XMFLOAT4(v3Binormal.x, v3Binormal.y, v3Binormal.z, fHeight * 0.5f - fd);
	m_v4TopPlane = XMFLOAT4(-v3Binormal.x, -v3Binormal.y, -v3Binormal.z, fHeight * 0.5f + fd);

	fd = XMVectorGetX(XMVector3Dot(XMLoadFloat3(&m_v3Center), XMLoadFloat3(&m_v3Normal)));
	m_v4FrontPlane = XMFLOAT4(-m_v3Normal.x, -m_v3Normal.y, -m_v3Normal.z, fDepth + fd);
	m_v4BackPlane = XMFLOAT4(m_v3Normal.x, m_v3Normal.y, m_v3Normal.z, fDepth - fd);

	m_dwVertexCount = 0;
	m_dwPrimitiveCount = 0;

	float fSearchRadius = fMAX(fWidth, fHeight);
	float fMinX = v3Center.x - fSearchRadius;
	float fMaxX = v3Center.x + fSearchRadius;
	float fMinY = fabsf(v3Center.y) - fSearchRadius;
	float fMaxY = fabsf(v3Center.y) + fSearchRadius;

	DWORD dwAffectedPrimitiveCount = 0;
	XMFLOAT3 v3AffectedVertex[MAX_SEARCH_VERTICES];
	XMFLOAT3 v3AffectedNormal[MAX_SEARCH_VERTICES];

	memset(v3AffectedVertex, 0, sizeof(v3AffectedVertex));
	memset(v3AffectedNormal, 0, sizeof(v3AffectedNormal));

	SearchAffectedTerrainMesh(fMinX, fMaxX, fMinY, fMaxY, &dwAffectedPrimitiveCount, v3AffectedVertex, v3AffectedNormal);

	ClipMesh(dwAffectedPrimitiveCount, v3AffectedVertex, v3AffectedNormal);

	float fOne_over_w = 1.0f / fWidth;
	float fOne_over_h = 1.0f / fHeight;

	for (DWORD dwi = 0; dwi < m_dwVertexCount; ++dwi)
	{
		XMFLOAT3 v3 = {
			m_Vertices[dwi].position.x - m_v3Center.x,
			m_Vertices[dwi].position.y - m_v3Center.y,
			m_Vertices[dwi].position.z - m_v3Center.z
		};

		float fu = -XMVectorGetX(XMVector3Dot(XMLoadFloat3(&v3), XMLoadFloat3(&v3Binormal))) * fOne_over_w + 0.5f;
		float fv = -XMVectorGetX(XMVector3Dot(XMLoadFloat3(&v3), XMLoadFloat3(&v3Tangent))) * fOne_over_h + 0.5f;

		m_Vertices[dwi].texCoord = XMFLOAT2(fu, fv);
	}
}

/*
void CTerrainDecal::Update()
{
}
*/

void CTerrainDecal::Render()
{
	STATEMANAGER.GetStateCache().Push();
	STATEMANAGER.GetSampler().Push(0);

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetSampler().SetAddressUV(0, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	CDecal::Render();

	STATEMANAGER.GetSampler().Restore(0);
	STATEMANAGER.GetStateCache().Restore();
}

void CTerrainDecal::SearchAffectedTerrainMesh(float fMinX,
	float fMaxX,
	float fMinY,
	float fMaxY,
	DWORD* pdwAffectedPrimitiveCount,
	XMFLOAT3* pv3AffectedVertex,
	XMFLOAT3* pv3AffectedNormal)
{
	if (!m_pMapOutdoor)
		return;

	int iMinX, iMaxX, iMinY, iMaxY;

	PR_FLOAT_TO_INT(fMinX, iMinX);
	PR_FLOAT_TO_INT(fMaxX, iMaxX);
	PR_FLOAT_TO_INT(fMinY, iMinY);
	PR_FLOAT_TO_INT(fMaxY, iMaxY);

	iMinX -= iMinX % CTerrainImpl::CELLSCALE;
	iMaxX -= iMaxX % CTerrainImpl::CELLSCALE;
	iMinY -= iMinY % CTerrainImpl::CELLSCALE;
	iMaxY -= iMaxY % CTerrainImpl::CELLSCALE;

	for (int iy = iMinY; iy <= iMaxY; iy += CTerrainImpl::CELLSCALE)
	{
		if (iy < 0)
			continue;

		WORD wTerrainNumY = iy / CTerrainImpl::TERRAIN_YSIZE;

		for (int ix = iMinX; ix <= iMaxX; ix += CTerrainImpl::CELLSCALE)
		{
			if (ix < 0)
				continue;

			WORD wTerrainNumX = ix / CTerrainImpl::TERRAIN_YSIZE;

			BYTE byTerrainNum;

			if (!m_pMapOutdoor->GetTerrainNumFromCoord(wTerrainNumX, wTerrainNumY, &byTerrainNum))
				continue;

			CTerrain* pTerrain;

			if (!m_pMapOutdoor->GetTerrainPointer(byTerrainNum, &pTerrain))
				continue;

			float fHeightLT = pTerrain->GetHeight(ix, iy) + m_cfDecalEpsilon;
			float fHeightRT = pTerrain->GetHeight(ix + CTerrainImpl::CELLSCALE, iy) + m_cfDecalEpsilon;
			float fHeightLB = pTerrain->GetHeight(ix, iy + CTerrainImpl::CELLSCALE) + m_cfDecalEpsilon;
			float fHeightRB = pTerrain->GetHeight(ix + CTerrainImpl::CELLSCALE, iy + CTerrainImpl::CELLSCALE) + m_cfDecalEpsilon;

			*pdwAffectedPrimitiveCount += 2;

			*pv3AffectedVertex++ = XMFLOAT3((float)ix, (float)(-iy), fHeightLT);
			*pv3AffectedVertex++ = XMFLOAT3((float)ix, (float)(-iy - CTerrainImpl::CELLSCALE), fHeightLB);
			*pv3AffectedVertex++ = XMFLOAT3((float)(ix + CTerrainImpl::CELLSCALE), (float)(-iy), fHeightRT);
			*pv3AffectedVertex++ = XMFLOAT3((float)(ix + CTerrainImpl::CELLSCALE), (float)(-iy), fHeightRT);
			*pv3AffectedVertex++ = XMFLOAT3((float)ix, (float)(-iy - CTerrainImpl::CELLSCALE), fHeightLB);
			*pv3AffectedVertex++ = XMFLOAT3((float)(ix + CTerrainImpl::CELLSCALE), (float)(-iy - CTerrainImpl::CELLSCALE), fHeightRB);

			*pv3AffectedNormal++ = XMFLOAT3(0.0f, 0.0f, 1.0f);
			*pv3AffectedNormal++ = XMFLOAT3(0.0f, 0.0f, 1.0f);
			*pv3AffectedNormal++ = XMFLOAT3(0.0f, 0.0f, 1.0f);
			*pv3AffectedNormal++ = XMFLOAT3(0.0f, 0.0f, 1.0f);
			*pv3AffectedNormal++ = XMFLOAT3(0.0f, 0.0f, 1.0f);
			*pv3AffectedNormal++ = XMFLOAT3(0.0f, 0.0f, 1.0f);
		}
	}
}
