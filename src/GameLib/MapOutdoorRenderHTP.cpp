#include "StdAfx.h"
#include "MapOutdoor.h"

#include "EterLib/StateManager.h"

namespace
{
	struct STerrainRenderInfo
	{
		DWORD fogColor = 0xffffffff;
		float fogNear = 5000.0f;
		float fogFar = 10000.0f;
		bool fogEnable = false;
		bool densityFog = false;
	};
}

void CMapOutdoor::__RenderTerrain_RenderHardwareTransformPatch(const RenderFrameContext& ctx)
{
	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	cb->SetFogEnable(ctx.FogEnable);
	cb->SetFogColor(ctx.FogColor);
	cb->SetFogStart(ctx.FogStart);
	cb->SetFogEnd(ctx.FogEnd);

	state.Push();

	state.Blend.SetBlendEnable(true);
	cb->SetAlphaTestEnable(true);
	cb->SetAlphaRef(0x00000000);
	cb->SetTextureFactor(ctx.FogColor);

	state.Sampler.SetAddressUV(0, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	state.Sampler.SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	state.Sampler.SetFilter(0, D3D11_FILTER_ANISOTROPIC);
	state.Sampler.SetMaxAnisotropy(0, 8);
	state.Sampler.SetFilter(1, D3D11_FILTER_ANISOTROPIC);
	state.Sampler.SetMaxAnisotropy(1, 8);

	CSpeedTreeWrapper::ms_bSelfShadowOn = true;

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.GetTransform().SetWorld(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture0(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture1(m_matWorldForCommonUse);

	m_iRenderedSplatNumSqSum = 0;
	m_iRenderedPatchNum = 0;
	m_iRenderedSplatNum = 0;
	m_RenderedTextureNumVector.clear();

	const std::pair<float, long> fogNearLimit(ctx.FogStart - 3200.0f, 0);
	std::pair<float, long> fogFarLimit(ctx.FogEnd + 1600.0f, 0);
	if (ctx.DensityFog)
		fogFarLimit.first = 1e10f;

	auto nearIt = std::upper_bound(m_PatchVector.begin(), m_PatchVector.end(), fogNearLimit);
	auto farIt = std::upper_bound(m_PatchVector.begin(), m_PatchVector.end(), fogFarLimit);

	WORD primitiveCount = 0;
	D3D11_PRIMITIVE_TOPOLOGY primitiveType = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	BYTE lodLevel = 0;

	const float lod1Distance = __GetNoFogDistance();
	const float lod2Distance = __GetFogDistance();

	SelectIndexBuffer(0, &primitiveCount, &primitiveType);

	auto updateLod = [&](float distance)
	{
		if (lodLevel == 0 && lod1Distance <= distance)
		{
			lodLevel = 1;
			SelectIndexBuffer(1, &primitiveCount, &primitiveType);
		}
		else if (lodLevel == 1 && lod2Distance <= distance)
		{
			lodLevel = 2;
			SelectIndexBuffer(2, &primitiveCount, &primitiveType);
		}
	};

	auto renderSplatRange = [&](auto beginIt, auto endIt)
	{
		for (auto it = beginIt; it != endIt; ++it)
		{
			updateLod(it->first);
			__HardwareTransformPatch_RenderPatchSplat(ctx, it->second, primitiveCount, primitiveType);

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, primitiveCount, primitiveType);

			if (m_iRenderedSplatNum >= m_iSplatLimit)
				return false;
		}
		return true;
	};

	if (renderSplatRange(m_PatchVector.begin(), nearIt) && m_iRenderedSplatNum < m_iSplatLimit)
		renderSplatRange(nearIt, farIt);

	cb->SetLightingEnable(false);
	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
	_mgr->SetShader(VF_TERRAIN, TERRAIN_BASE);

	if (m_iRenderedSplatNum < m_iSplatLimit)
	{
		for (auto it = farIt; it != m_PatchVector.end(); ++it)
		{
			updateLod(it->first);
			__HardwareTransformPatch_RenderPatchNone(it->second, primitiveCount, primitiveType);

			if (m_bDrawWireFrame)
				DrawWireFrame(it->second, primitiveCount, primitiveType);
		}
	}

	cb->SetLightingEnable(true);
	std::sort(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end());

	state.Restore();
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchSplat(const RenderFrameContext& ctx, long patchnum, WORD wPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType)
{
	assert(NULL != m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchSplat");

	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	if (!pTerrainPatchProxy->isUsed())
		return;

	const long patchLocalNum = pTerrainPatchProxy->GetPatchNum();
	if (patchLocalNum < 0)
		return;

	const BYTE terrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (terrainNum == 0xFF)
		return;

	CTerrain* pTerrain = nullptr;
	if (!GetTerrainPointer(terrainNum, &pTerrain))
		return;

	auto pkVB = pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr();
	if (!pkVB)
		return;

	const DWORD fogColor = ctx.FogColor;

	WORD coordX = 0;
	WORD coordY = 0;
	pTerrain->GetCoordinate(&coordX, &coordY);

	D3DXMATRIX matTexTransform;
	D3DXMATRIX matSplatAlphaTexTransform;
	D3DXMATRIX matSplatColorTexTransform;

	m_matWorldForCommonUse._41 = -float(coordX * CTerrainImpl::TERRAIN_XSIZE);
	m_matWorldForCommonUse._42 = float(coordY * CTerrainImpl::TERRAIN_YSIZE);

	D3DXMatrixMultiply(&matTexTransform, &ctx.ViewInverse, &m_matWorldForCommonUse);
	D3DXMatrixMultiply(&matSplatAlphaTexTransform, &matTexTransform, &m_matSplatAlpha);

	STATEMANAGER.GetTransform().SetTexture1(matSplatAlphaTexTransform);
	_mgr->SetVertexBuffer(pkVB);
	_mgr->SetShader(VF_TERRAIN, TERRAIN_SPLAT);
	cb->SetLightingEnable(false);

	TTerrainSplatPatch& rTerrainSplatPatch = pTerrain->GetTerrainSplatPatch();
	const int prevRenderedSplatNum = m_iRenderedSplatNum;

	for (DWORD j = 1; j < pTerrain->GetNumTextures(); ++j)
	{
		TTerainSplat& rSplat = rTerrainSplatPatch.Splats[j];
		if (!rSplat.Active || rTerrainSplatPatch.PatchTileCount[patchLocalNum][j] == 0)
			continue;

		const TTerrainTexture& rTexture = m_TextureSet.GetTexture(j);
		D3DXMatrixMultiply(&matSplatColorTexTransform, &ctx.ViewInverse, &rTexture.m_matTransform);

		STATEMANAGER.GetTransform().SetTexture0(matSplatColorTexTransform);
		STATEMANAGER.SetTexture(0, rTexture.pd3dTexture);
		STATEMANAGER.SetTexture(1, rSplat.pd3dTexture);
		STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);

		if (std::find(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end(), int(j)) == m_RenderedTextureNumVector.end())
			m_RenderedTextureNumVector.push_back(int(j));

		++m_iRenderedSplatNum;
		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;
	}

	if (ctx.DrawShadow)
	{
		state.Push();

		cb->SetLightingEnable(true);
		cb->SetFogColor(0xFFFFFFFF);

		state.Blend.SetSrcBlend(D3D11_BLEND_ZERO);
		state.Blend.SetDestBlend(D3D11_BLEND_SRC_COLOR);
		state.Sampler.SetAddressUV(0, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

		D3DXMATRIX matShadowTexTransform;
		D3DXMatrixMultiply(&matShadowTexTransform, &matTexTransform, &m_matStaticShadow);

		STATEMANAGER.GetTransform().SetTexture0(matShadowTexTransform);
		STATEMANAGER.SetTexture(0, pTerrain->GetShadowTexture());

		if (ctx.DrawCharacterShadow && ctx.CharacterShadowTexture)
		{
			STATEMANAGER.GetTransform().SetTexture1(ctx.DynamicShadowMatrix);
			STATEMANAGER.SetTexture(1, ctx.CharacterShadowTexture);
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

		cb->SetFogColor(fogColor);
		cb->SetLightingEnable(false);
		_mgr->SetShader(VF_TERRAIN, TERRAIN_SPLAT);
	}

	++m_iRenderedPatchNum;

	const int renderedSplatCount = m_iRenderedSplatNum - prevRenderedSplatNum;
	m_iRenderedSplatNumSqSum += renderedSplatCount * renderedSplatCount;
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

	_mgr->SetVertexBuffer(pkVB);
	STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);
}
