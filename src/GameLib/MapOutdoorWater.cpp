#include "StdAfx.h"
#include "EterLib/StateManager.h"
#include "EterLib/ResourceManager.h"

#include "MapOutdoor.h"
#include "TerrainPatch.h"

void CMapOutdoor::LoadWaterTexture()
{
	UnloadWaterTexture();
	char buf[256];
	for (int i = 0; i < 30; ++i)
	{
		sprintf(buf, "d:/ymir Work/special/water/%02d.dds", i+1);
		m_WaterInstances[i].SetImagePointer(CResourceManager::Instance().GetTyped<CGraphicImage>(buf));
	}
}

void CMapOutdoor::UnloadWaterTexture()
{
	for (int i = 0; i < 30; ++i)
		m_WaterInstances[i].Destroy();
}

void CMapOutdoor::RenderWater(const RenderFrameContext& ctx)
{
    if (m_PatchVector.empty())
        return;

    if (!IsVisiblePart(PART_WATER))
        return;

    auto& state = STATEMANAGER.GetStateCache();

    D3DXMATRIX matTexTransformWater;

    state.Push();
    STATEMANAGER.GetSampler().Push(0);

    STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(false);

    STATEMANAGER.GetBlend().SetBlendEnable(true);
    STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

    const uint32_t frame = static_cast<uint32_t>(ctx.Time * 1000.0f / 70.0f) % 30;

    STATEMANAGER.SetTexture(0, m_WaterInstances[frame].GetTexturePointer()->GetSRV());

    D3DXMatrixScaling(&matTexTransformWater, m_fWaterTexCoordBase, -m_fWaterTexCoordBase, 0.0f);

    D3DXMatrixMultiply(&matTexTransformWater, &ctx.ViewInverse, &matTexTransformWater);

    STATEMANAGER.GetTransform().Push();
    STATEMANAGER.GetTransform().SetTexture0(matTexTransformWater);

    STATEMANAGER.GetSampler().SetFilter(0, D3D11_FILTER_ANISOTROPIC);
    STATEMANAGER.GetSampler().SetMaxAnisotropy(0, 8);

    STATEMANAGER.GetSampler().SetAddressUV(0, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

    STATEMANAGER.SetTexture(1, nullptr);

    static float s_fWaterHeightCurrent = 0.0f;
    static float s_fWaterHeightBegin = 0.0f;
    static float s_fWaterHeightEnd = 0.0f;

    static float s_fBlendElapsed = 0.0f;
    static float s_fBlendDuration = 2.0f;

    s_fBlendElapsed += ctx.DeltaTime;

    if (s_fBlendElapsed >= s_fBlendDuration)
    {
        s_fBlendDuration = random_range(1.0f, 3.0f);

        s_fWaterHeightEnd = (s_fWaterHeightEnd == 0.0f) ? -random_range(0.0f, 15.0f) : 0.0f;

        s_fWaterHeightBegin = s_fWaterHeightCurrent;
        s_fBlendElapsed = 0.0f;
    }

    const float t = std::clamp(s_fBlendElapsed / s_fBlendDuration, 0.0f, 1.0f);

    s_fWaterHeightCurrent = s_fWaterHeightBegin + (s_fWaterHeightEnd - s_fWaterHeightBegin) * t;

    m_matWorldForCommonUse._41 = 0.0f;
    m_matWorldForCommonUse._42 = 0.0f;
    m_matWorldForCommonUse._43 = s_fWaterHeightCurrent;

    STATEMANAGER.GetTransform().SetWorld(m_matWorldForCommonUse);

    const float fogDistance = ctx.FogEnd;

    for (const auto& patch : m_PatchVector)
    {
        if (patch.first < fogDistance)
            DrawWater(patch.second);
    }

    STATEMANAGER.SetTexture(0, nullptr);
    STATEMANAGER.GetBlend().SetBlendEnable(false);

    for (const auto& patch : m_PatchVector)
    {
        if (patch.first >= fogDistance)
            DrawWater(patch.second);
    }

    m_matWorldForCommonUse._43 = 0.0f;

    STATEMANAGER.GetTransform().Restore();
    STATEMANAGER.GetSampler().Restore(0);
    state.Restore();
}

void CMapOutdoor::DrawWater(long patchnum)
{
	assert(NULL!=m_pTerrainPatchProxyList);
	if (!m_pTerrainPatchProxyList)
		return;

	CTerrainPatchProxy& rkTerrainPatchProxy = m_pTerrainPatchProxyList[patchnum];

	if (!rkTerrainPatchProxy.isUsed())
		return;

	if (!rkTerrainPatchProxy.isWaterExists())
		return;

	auto pkVB=rkTerrainPatchProxy.GetWaterVertexBufferPointer();
	if (!pkVB)
		return;
	
	UINT uPriCount=rkTerrainPatchProxy.GetWaterFaceCount();
	if (!uPriCount)
		return;
	
	_mgr->SetShader(VF_PD);
	_mgr->SetVertexBuffer(pkVB);

	STATEMANAGER.DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST, uPriCount, 0);

	ms_faceCount += uPriCount;
}
