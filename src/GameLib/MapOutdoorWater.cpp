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
		m_WaterInstances[i].SetImagePointer((CGraphicImage *) CResourceManager::Instance().GetResourcePointer(buf));
	}
}

void CMapOutdoor::UnloadWaterTexture()
{
	for (int i = 0; i < 30; ++i)
		m_WaterInstances[i].Destroy();
}

void CMapOutdoor::RenderWater()
{
	if (m_PatchVector.empty())
		return;

	if (!IsVisiblePart(PART_WATER))
		return;

	D3DXMATRIX matTexTransformWater;

	STATEMANAGER.GetStateCache().Push();
	STATEMANAGER.GetSampler().Push(0);

	STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(false);
	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);

	STATEMANAGER.SetTexture(0, m_WaterInstances[((ELTimer_GetMSec() / 70) % 30)].GetTexturePointer()->GetSRV());

	D3DXMatrixScaling(&matTexTransformWater, m_fWaterTexCoordBase, -m_fWaterTexCoordBase, 0.0f);
	D3DXMatrixMultiply(&matTexTransformWater, &m_matViewInverse, &matTexTransformWater);

	STATEMANAGER.GetTransform().Push();
	STATEMANAGER.GetTransform().SetTexture0(matTexTransformWater);

	STATEMANAGER.GetSampler().SetFilter(0, D3D11_FILTER_ANISOTROPIC);
	STATEMANAGER.GetSampler().SetMaxAnisotropy(0, 8);
	STATEMANAGER.GetSampler().SetAddressUV(0, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	STATEMANAGER.SetTexture(1, NULL);

	static float s_fWaterHeightCurrent = 0;
	static float s_fWaterHeightBegin = 0;
	static float s_fWaterHeightEnd = 0;
	static DWORD s_dwLastHeightChangeTime = CTimer::Instance().GetCurrentMillisecond();
	static DWORD s_dwBlendtime = 300;

	if ((CTimer::Instance().GetCurrentMillisecond() - s_dwLastHeightChangeTime) > s_dwBlendtime)
	{
		s_dwBlendtime = random_range(1000, 3000);

		if (s_fWaterHeightEnd == 0)
			s_fWaterHeightEnd = -random_range(0, 15);
		else
			s_fWaterHeightEnd = 0;

		s_fWaterHeightBegin = s_fWaterHeightCurrent;
		s_dwLastHeightChangeTime = CTimer::Instance().GetCurrentMillisecond();
	}

	s_fWaterHeightCurrent = s_fWaterHeightBegin + (s_fWaterHeightEnd - s_fWaterHeightBegin) * (float)((CTimer::Instance().GetCurrentMillisecond() - s_dwLastHeightChangeTime) / (float)s_dwBlendtime);
	m_matWorldForCommonUse._43 = s_fWaterHeightCurrent;

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;
	STATEMANAGER.GetTransform().SetWorld(m_matWorldForCommonUse);

	float fFogDistance = __GetFogDistance();

	std::vector<std::pair<float, long>>::iterator i;

	for (i = m_PatchVector.begin(); i != m_PatchVector.end(); ++i)
	{
		if (i->first < fFogDistance)
			DrawWater(i->second);
	}

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.GetBlend().SetBlendEnable(false);

	for (i = m_PatchVector.begin(); i != m_PatchVector.end(); ++i)
	{
		if (i->first >= fFogDistance)
			DrawWater(i->second);
	}

	m_matWorldForCommonUse._43 = 0.0f;

	STATEMANAGER.GetTransform().Restore();
	STATEMANAGER.GetSampler().Restore(0);
	STATEMANAGER.GetStateCache().Restore();
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
