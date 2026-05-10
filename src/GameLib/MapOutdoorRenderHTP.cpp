#include "StdAfx.h"
#include "MapOutdoor.h"

#include "EterLib/StateManager.h"

void CMapOutdoor::__RenderTerrain_RenderHardwareTransformPatch()
{
	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	DWORD dwFogColor;
	float fFogFarDistance;
	float fFogNearDistance;

	if (mc_pEnvironmentData)
	{
		dwFogColor = mc_pEnvironmentData->FogColor;
		fFogNearDistance = mc_pEnvironmentData->GetFogNearDistance();
		fFogFarDistance = mc_pEnvironmentData->GetFogFarDistance();
	}
	else
	{
		dwFogColor = 0xffffffff;
		fFogNearDistance = 5000.0f;
		fFogFarDistance = 10000.0f;
	}

	state.Push();

	state.Blend.SetBlendEnable(true);
	cb->SetAlphaTestEnable(true);
	cb->SetAlphaRef(0x00000000);
	cb->SetTextureFactor(dwFogColor);

	state.Sampler.SetAddressUV(0, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	state.Sampler.SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	CSpeedTreeWrapper::ms_bSelfShadowOn = true;

	state.Sampler.SetFilter(0, D3D11_FILTER_ANISOTROPIC);
	state.Sampler.SetMaxAnisotropy(0, 8);
	state.Sampler.SetFilter(1, D3D11_FILTER_ANISOTROPIC);
	state.Sampler.SetMaxAnisotropy(1, 8);

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.GetTransform().SetWorld(m_matWorldForCommonUse);

	STATEMANAGER.GetTransform().Push();
	STATEMANAGER.GetTransform().SetTexture0(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture1(m_matWorldForCommonUse);

	m_iRenderedSplatNumSqSum = 0;
	m_iRenderedPatchNum = 0;
	m_iRenderedSplatNum = 0;
	m_RenderedTextureNumVector.clear();

	std::pair<float, long> fog_far(fFogFarDistance + 1600.0f, 0);
	std::pair<float, long> fog_near(fFogNearDistance - 3200.0f, 0);

	if (mc_pEnvironmentData && mc_pEnvironmentData->bDensityFog)
		fog_far.first = 1e10f;

	auto far_it = std::upper_bound(m_PatchVector.begin(), m_PatchVector.end(), fog_far);
	auto near_it = std::upper_bound(m_PatchVector.begin(), m_PatchVector.end(), fog_near);

	WORD wPrimitiveCount;
	D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType;

	BYTE byCUrrentLODLevel = 0;

	float fLODLevel1Distance = __GetNoFogDistance();
	float fLODLevel2Distance = __GetFogDistance();

	SelectIndexBuffer(0, &wPrimitiveCount, &ePrimitiveType);

	auto it = m_PatchVector.begin();

	for (; it != near_it; ++it)
	{
		if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
		{
			byCUrrentLODLevel = 1;
			SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
		}
		else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
		{
			byCUrrentLODLevel = 2;
			SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
		}

		__HardwareTransformPatch_RenderPatchSplat(it->second, wPrimitiveCount, ePrimitiveType);

		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;

		if (m_bDrawWireFrame)
			DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
	}

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for (it = near_it; it != far_it; ++it)
		{
			if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
			{
				byCUrrentLODLevel = 1;
				SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
			}
			else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
			{
				byCUrrentLODLevel = 2;
				SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
			}

			__HardwareTransformPatch_RenderPatchSplat(it->second, wPrimitiveCount, ePrimitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				break;

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
		}
	}

	cb->SetLightingEnable(false);
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);

	_mgr->SetShader(VF_TERRAIN, TERRAIN_SPLAT);

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for (it = far_it; it != m_PatchVector.end(); ++it)
		{
			if (byCUrrentLODLevel == 0 && fLODLevel1Distance <= it->first)
			{
				byCUrrentLODLevel = 1;
				SelectIndexBuffer(1, &wPrimitiveCount, &ePrimitiveType);
			}
			else if (byCUrrentLODLevel == 1 && fLODLevel2Distance <= it->first)
			{
				byCUrrentLODLevel = 2;
				SelectIndexBuffer(2, &wPrimitiveCount, &ePrimitiveType);
			}

			__HardwareTransformPatch_RenderPatchNone(it->second, wPrimitiveCount, ePrimitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				break;

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, wPrimitiveCount, ePrimitiveType);
		}
	}

	cb->SetLightingEnable(true);

	std::sort(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end());

	STATEMANAGER.GetTransform().Restore();
	state.Restore();
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchSplat(long patchnum, WORD wPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType)
{
	assert(NULL != m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchSplat");

	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];

	if (!pTerrainPatchProxy->isUsed())
		return;

	long sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	BYTE ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (ucTerrainNum == 0xFF)
		return;

	CTerrain* pTerrain;
	if (!GetTerrainPointer(ucTerrainNum, &pTerrain))
		return;

	DWORD dwFogColor = mc_pEnvironmentData->FogColor;

	WORD wCoordX, wCoordY;
	pTerrain->GetCoordinate(&wCoordX, &wCoordY);

	TTerrainSplatPatch& rTerrainSplatPatch = pTerrain->GetTerrainSplatPatch();

	D3DXMATRIX matTexTransform;
	D3DXMATRIX matSplatAlphaTexTransform;
	D3DXMATRIX matSplatColorTexTransform;

	m_matWorldForCommonUse._41 = -float(wCoordX * CTerrainImpl::TERRAIN_XSIZE);
	m_matWorldForCommonUse._42 = float(wCoordY * CTerrainImpl::TERRAIN_YSIZE);

	D3DXMatrixMultiply(&matTexTransform, &m_matViewInverse, &m_matWorldForCommonUse);
	D3DXMatrixMultiply(&matSplatAlphaTexTransform, &matTexTransform, &m_matSplatAlpha);

	STATEMANAGER.GetTransform().SetTexture1(matSplatAlphaTexTransform);
	auto pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	_mgr->SetVertexBuffer(pkVB);

	cb->SetLightingEnable(false);

	int iPrevRenderedSplatNum = m_iRenderedSplatNum;
	bool isFirst = true;

	for (DWORD j = 1; j < pTerrain->GetNumTextures(); ++j)
	{
		TTerainSplat& rSplat = rTerrainSplatPatch.Splats[j];

		if (!rSplat.Active)
			continue;

		if (rTerrainSplatPatch.PatchTileCount[sPatchNum][j] == 0)
			continue;

		const TTerrainTexture& rTexture = m_TextureSet.GetTexture(j);

		D3DXMatrixMultiply(&matSplatColorTexTransform, &m_matViewInverse, &rTexture.m_matTransform);
		STATEMANAGER.GetTransform().SetTexture0(matSplatColorTexTransform);

		STATEMANAGER.SetTexture(0, rTexture.pd3dTexture);
		STATEMANAGER.SetTexture(1, rSplat.pd3dTexture);

		_mgr->SetShader(VF_TERRAIN, TERRAIN_SPLAT);
		isFirst = false;

		STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);

		auto aIterator = std::find(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end(), int(j));
		if (aIterator == m_RenderedTextureNumVector.end())
			m_RenderedTextureNumVector.push_back(int(j));

		++m_iRenderedSplatNum;

		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;
	}

	if (m_bDrawShadow)
	{
		state.Push();

		cb->SetLightingEnable(true);
		cb->SetFogColor(0xFFFFFFFF);

		state.Blend.SetSrcBlend(D3D11_BLEND_ZERO);
		state.Blend.SetDestBlend(D3D11_BLEND_SRC_COLOR);

		D3DXMATRIX matShadowTexTransform;
		D3DXMatrixMultiply(&matShadowTexTransform, &matTexTransform, &m_matStaticShadow);

		STATEMANAGER.GetTransform().SetTexture0(matShadowTexTransform);

		STATEMANAGER.SetTexture(0, pTerrain->GetShadowTexture());

		state.Sampler.SetAddressUV(0, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

		if (m_bDrawChrShadow)
		{
			STATEMANAGER.GetTransform().SetTexture1(m_matDynamicShadow);

			STATEMANAGER.SetTexture(1, m_lpCharacterShadowMapTexture);
			state.Sampler.SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

			_mgr->SetShader(VF_TERRAIN, TERRAIN_SHADOW | TERRAIN_SHADOW_CHR);
		}
		else
		{
			STATEMANAGER.SetTexture(1, NULL);
			_mgr->SetShader(VF_TERRAIN, TERRAIN_SHADOW);
		}

		ms_faceCount += wPrimitiveCount;
		STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);
		++m_iRenderedSplatNum;

		state.Restore();

		cb->SetFogColor(dwFogColor);
		cb->SetLightingEnable(false);
	}

	++m_iRenderedPatchNum;

	int iCurRenderedSplatNum = m_iRenderedSplatNum - iPrevRenderedSplatNum;
	m_iRenderedSplatNumSqSum += iCurRenderedSplatNum * iCurRenderedSplatNum;
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchNone(long patchnum, WORD wPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType)
{
	assert(NULL != m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchNone");
	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];

	if (!pTerrainPatchProxy->isUsed())
		return;

	auto pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	_mgr->SetShader(VF_TERRAIN, TERRAIN_BASE);
	_mgr->SetVertexBuffer(pkVB);
	STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);
}
