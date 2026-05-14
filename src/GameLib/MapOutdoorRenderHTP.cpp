#include "StdAfx.h"
#include "MapOutdoor.h"

#include "EterLib/ResourceManager.h"
#include "EterLib/StateManager.h"

namespace
{
	struct STerrainPatchLodState
	{
		BYTE lodLevel;
		WORD primitiveCount;
		D3D11_PRIMITIVE_TOPOLOGY primitiveType;
	};

	inline bool TerrainTextureWasRendered(const std::vector<int>& textures, int textureIndex)
	{
		return std::find(textures.begin(), textures.end(), textureIndex) != textures.end();
	}

	inline ID3D11ShaderResourceView* GetWhiteTerrainTexture()
	{
		CGraphicImage* image = CResourceManager::Instance().GetTyped<CGraphicImage>("d:/ymir work/special/white.dds");
		if (!image)
			return NULL;

		CGraphicTexture* texture = image->GetTexturePointer();
		if (!texture || texture->IsEmpty())
			return NULL;

		return texture->GetSRV();
	}
}

void CMapOutdoor::__RenderTerrain_RenderHardwareTransformPatch(const RenderContext& ctx)
{
	auto& state = STATEMANAGER.GetStateCache();
	auto cb = _mgr->GetCbMgr();

	cb->SetFogEnable(ctx.Frame.FogEnable);
	cb->SetFogColor(ctx.Frame.FogColor);
	cb->SetFogStart(ctx.Frame.FogStart);
	cb->SetFogEnd(ctx.Frame.FogEnd);

	state.Push();

	state.Blend.SetBlendEnable(true);
	cb->SetAlphaTestEnable(true);
	cb->SetAlphaRef(0x00000000);
	cb->SetTextureFactor(ctx.Frame.FogColor);
	cb->SetLightingEnable(false);

	state.Sampler.SetAddressUV(0, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
	state.Sampler.SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	state.Sampler.SetAddressUV(2, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
	state.Sampler.SetAddressUV(3, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	state.Sampler.SetFilter(0, D3D11_FILTER_ANISOTROPIC);
	state.Sampler.SetMaxAnisotropy(0, 8);
	state.Sampler.SetFilter(1, D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	state.Sampler.SetFilter(2, D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	state.Sampler.SetFilter(3, D3D11_FILTER_MIN_MAG_MIP_LINEAR);

	CSpeedTreeWrapper::ms_bSelfShadowOn = true;

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.GetTransform().SetWorld(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture0(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture1(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture2(m_matWorldForCommonUse);
	STATEMANAGER.GetTransform().SetTexture3(m_matWorldForCommonUse);

	m_iRenderedSplatNumSqSum = 0;
	m_iRenderedPatchNum = 0;
	m_iRenderedSplatNum = 0;
	m_RenderedTextureNumVector.clear();

	STerrainPatchLodState lodState{};
	lodState.lodLevel = 0;
	SelectIndexBuffer(0, &lodState.primitiveCount, &lodState.primitiveType);

	const float lod1Distance = __GetNoFogDistance();
	const float lod2Distance = __GetFogDistance();

	auto updateLod = [&](float distance)
		{
			if (lodState.lodLevel == 0 && lod1Distance <= distance)
			{
				lodState.lodLevel = 1;
				SelectIndexBuffer(1, &lodState.primitiveCount, &lodState.primitiveType);
			}
			else if (lodState.lodLevel == 1 && lod2Distance <= distance)
			{
				lodState.lodLevel = 2;
				SelectIndexBuffer(2, &lodState.primitiveCount, &lodState.primitiveType);
			}
		};

	_mgr->SetShader(VF_TERRAIN);

	for (auto it = m_PatchVector.begin(); it != m_PatchVector.end(); ++it)
	{
		updateLod(it->first);
		__HardwareTransformPatch_RenderPatchSplat(ctx, it->second, lodState.primitiveCount, lodState.primitiveType);

		if (m_bDrawWireFrame)
			DrawWireFrame(it->second, lodState.primitiveCount, lodState.primitiveType);

		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;
	}

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.SetTexture(2, NULL);
	STATEMANAGER.SetTexture(3, NULL);

	cb->SetLightingEnable(true);
	std::sort(m_RenderedTextureNumVector.begin(), m_RenderedTextureNumVector.end());

	state.Restore();
}

void CMapOutdoor::__HardwareTransformPatch_RenderPatchSplat(const RenderContext& ctx, long patchnum, WORD wPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType)
{
	assert(NULL != m_pTerrainPatchProxyList && "__HardwareTransformPatch_RenderPatchSplat");

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

	WORD coordX = 0;
	WORD coordY = 0;
	pTerrain->GetCoordinate(&coordX, &coordY);

	XMFLOAT4X4 matTexTransform;
	XMFLOAT4X4 matSplatAlphaTexTransform;
	XMFLOAT4X4 matSplatColorTexTransform;
	XMFLOAT4X4 matShadowTexTransform;

	m_matWorldForCommonUse._41 = -float(coordX * CTerrainImpl::TERRAIN_XSIZE);
	m_matWorldForCommonUse._42 = float(coordY * CTerrainImpl::TERRAIN_YSIZE);

	XMMATRIX viewInv = XMLoadFloat4x4(&ctx.Frame.ViewInverse);
	XMMATRIX world = XMLoadFloat4x4(&m_matWorldForCommonUse);

	XMMATRIX mat = viewInv * world;
	XMStoreFloat4x4(&matTexTransform, mat);

	XMMATRIX splatAlpha = mat * XMLoadFloat4x4(&m_matSplatAlpha);
	XMStoreFloat4x4(&matSplatAlphaTexTransform, splatAlpha);

	XMMATRIX shadow = mat * XMLoadFloat4x4(&m_matStaticShadow);
	XMStoreFloat4x4(&matShadowTexTransform, shadow);


	STATEMANAGER.GetTransform().SetTexture1(matSplatAlphaTexTransform);
	STATEMANAGER.GetTransform().SetTexture2(matShadowTexTransform);
	STATEMANAGER.GetTransform().SetTexture3(ctx.Frame.DynamicShadowMatrix);

	ID3D11ShaderResourceView* whiteTexture = GetWhiteTerrainTexture();
	ID3D11ShaderResourceView* staticShadowTexture = ctx.Frame.DrawShadow ? pTerrain->GetShadowTexture() : whiteTexture;
	ID3D11ShaderResourceView* characterShadowTexture = (ctx.Frame.DrawShadow && ctx.Frame.DrawCharacterShadow && ctx.Frame.CharacterShadowTexture) ? ctx.Frame.CharacterShadowTexture : whiteTexture;

	STATEMANAGER.SetTexture(2, staticShadowTexture);
	STATEMANAGER.SetTexture(3, characterShadowTexture);

	_mgr->SetVertexBuffer(pkVB);

	TTerrainSplatPatch& rTerrainSplatPatch = pTerrain->GetTerrainSplatPatch();
	const int prevRenderedSplatNum = m_iRenderedSplatNum;
	const DWORD textureCount = pTerrain->GetNumTextures();

	for (DWORD j = 1; j < textureCount; ++j)
	{
		TTerainSplat& rSplat = rTerrainSplatPatch.Splats[j];
		if (!rSplat.Active || rTerrainSplatPatch.PatchTileCount[patchLocalNum][j] == 0)
			continue;

		const TTerrainTexture& rTexture = m_TextureSet.GetTexture(j);

		XMMATRIX viewInv = XMLoadFloat4x4(&ctx.Frame.ViewInverse);
		XMMATRIX texTrans = XMLoadFloat4x4(&rTexture.m_matTransform);

		XMMATRIX result = viewInv * texTrans;

		XMStoreFloat4x4(&matSplatColorTexTransform, result);

		STATEMANAGER.GetTransform().SetTexture0(matSplatColorTexTransform);
		STATEMANAGER.SetTexture(0, rTexture.pd3dTexture);
		STATEMANAGER.SetTexture(1, rSplat.pd3dTexture);
		STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);

		if (!TerrainTextureWasRendered(m_RenderedTextureNumVector, int(j)))
			m_RenderedTextureNumVector.push_back(int(j));

		++m_iRenderedSplatNum;
		if (m_iRenderedSplatNum >= m_iSplatLimit)
			break;
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
