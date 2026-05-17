#include "StdAfx.h"
#include "MapOutdoor.h"
#include "TerrainPatch.h"
#include "AreaTerrain.h"
#include "TerrainQuadtree.h"

#include "EterLib/Camera.h"
#include "EterLib/StateManager.h"
#include "qMin32Lib/DxManager.h"
#include "EterBase/Timer.h"

CArea::TCRCWithNumberVector m_dwRenderedCRCWithNumberVector;

CMapOutdoor::TTerrainNumVector CMapOutdoor::FSortPatchDrawStructWithTerrainNum::m_TerrainNumVector;

RenderContext CMapOutdoor::BuildRenderFrameContext() const
{
	RenderContext ctx = RenderContext::Default();
	ctx.Frame.Device = ms_lpd3d11Device;
	ctx.Frame.DeviceContext = ms_lpd3d11Context;
	ctx.Frame.View = ms_matView;
	ctx.Frame.Projection = ms_matProj;
	XMMATRIX vp = XMMatrixMultiply(XMLoadFloat4x4(&ms_matView), XMLoadFloat4x4(&ms_matProj) );
	XMStoreFloat4x4(&ctx.Frame.ViewProjection, vp);
	ctx.Frame.ViewInverse = m_matViewInverse;
	ctx.Frame.DrawShadow = m_bDrawShadow;
	ctx.Frame.DrawCharacterShadow = m_bDrawChrShadow && m_lpCharacterShadowMapTexture != nullptr;
	ctx.Frame.CharacterShadowTexture = m_lpCharacterShadowMapTexture;
	ctx.Frame.DynamicShadowMatrix = m_matDynamicShadow;

	CCamera* camera = CCameraManager::Instance().GetCurrentCamera();
	if (camera)
	{
		ctx.Frame.Eye = camera->GetEye();
		ctx.Frame.Target = camera->GetTarget();
	}

	if (mc_pEnvironmentData)
	{
		ctx.Frame.FogEnable = mc_pEnvironmentData->bFogEnable;
		ctx.Frame.FogColor = ColorToUint(mc_pEnvironmentData->FogColor);
		ctx.Frame.FogStart = mc_pEnvironmentData->GetFogNearDistance();
		ctx.Frame.FogEnd = mc_pEnvironmentData->GetFogFarDistance();
		ctx.Frame.DensityFog = mc_pEnvironmentData->bDensityFog;
	}
	else
	{
		ctx.Frame.FogEnable = false;
		ctx.Frame.FogColor = 0xffffffff;
		ctx.Frame.FogStart = 5000.0f;
		ctx.Frame.FogEnd = 10000.0f;
	}

	static DWORD lastTime = CTimer::Instance().GetCurrentMillisecond();
	const DWORD now = CTimer::Instance().GetCurrentMillisecond();
	ctx.Frame.DeltaTime = static_cast<float>(now - lastTime) * 0.001f;
	ctx.Frame.Time = static_cast<float>(now) * 0.001f;
	lastTime = now;
	return ctx;
}

void CMapOutdoor::RenderTerrain(const RenderContext& ctx)
{
	if (!IsVisiblePart(PART_TERRAIN) || !m_bSettingTerrainVisible || !m_pTerrainPatchProxyList || !m_pRootNode)
		return;

	XMFLOAT4X4 viewProjection = ctx.Frame.ViewProjection;
	BuildViewFrustum(viewProjection);

	m_fXforDistanceCaculation = -ctx.Frame.Eye.x;
	m_fYforDistanceCaculation = -ctx.Frame.Eye.y;

	m_PatchVector.clear();
	const size_t reserveCount = static_cast<size_t>(m_wPatchCount) * static_cast<size_t>(m_wPatchCount);
	if (m_PatchVector.capacity() < reserveCount)
		m_PatchVector.reserve(reserveCount);

	__RenderTerrain_RecurseRenderQuadTree(m_pRootNode);
	if (m_PatchVector.empty())
		return;

	std::sort(m_PatchVector.begin(), m_PatchVector.end());
	__RenderTerrain_RenderHardwareTransformPatch(ctx);
}

void CMapOutdoor::__RenderTerrain_RecurseRenderQuadTree(CTerrainQuadtreeNode *Node, bool bCullCheckNeed)
{
	if (bCullCheckNeed)
	{
		switch (__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(Node->center, Node->radius))
		{
			case VIEW_ALL:
				// all child nodes need not cull check
				bCullCheckNeed = false;
				break;
			case VIEW_PART:
				break;
			case VIEW_NONE:
				// no need to render
				return;
		}
		// if no need cull check more
		// -> bCullCheckNeed = false;
	}
	
	if (Node->Size == 1)
	{
		XMFLOAT3 v3Center = Node->center;
		float fDistance = fMAX(fabs(v3Center.x + m_fXforDistanceCaculation), fabs(-v3Center.y + m_fYforDistanceCaculation));
		__RenderTerrain_AppendPatch(v3Center, fDistance, Node->PatchNum);
	}
	else
	{
		if (Node->NW_Node != NULL)
			__RenderTerrain_RecurseRenderQuadTree(Node->NW_Node, bCullCheckNeed);
		if (Node->NE_Node != NULL)
			__RenderTerrain_RecurseRenderQuadTree(Node->NE_Node, bCullCheckNeed);
		if (Node->SW_Node != NULL)
			__RenderTerrain_RecurseRenderQuadTree(Node->SW_Node, bCullCheckNeed);
		if (Node->SE_Node != NULL)
			__RenderTerrain_RecurseRenderQuadTree(Node->SE_Node, bCullCheckNeed);
	}
}

int CMapOutdoor::__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(const XMFLOAT3& c_v3Center, const float& c_fRadius)
{
	const int count = 6;

	XMFLOAT3 center = c_v3Center;
	center.y = -center.y;

	XMVECTOR vCenter = XMLoadFloat3(&center);

	float distance[6];

	for (int i = 0; i < count; ++i)
	{
		XMVECTOR plane = XMLoadFloat4(&m_plane[i]);
		distance[i] = XMVectorGetX(XMPlaneDotCoord(plane, vCenter));

		if (distance[i] <= -c_fRadius)
			return VIEW_NONE;
	}

	for (int i = 0; i < count; ++i)
	{
		if (distance[i] <= c_fRadius)
			return VIEW_PART;
	}

	return VIEW_ALL;
}

void CMapOutdoor::__RenderTerrain_AppendPatch(const XMFLOAT3& c_rv3Center, float fDistance, long lPatchNum)
{
	assert(NULL!=m_pTerrainPatchProxyList && "CMapOutdoor::__RenderTerrain_AppendPatch");
	if (!m_pTerrainPatchProxyList[lPatchNum].isUsed())
		return;

	m_pTerrainPatchProxyList[lPatchNum].SetCenterPosition(c_rv3Center);
	m_PatchVector.push_back(std::make_pair(fDistance, lPatchNum));
}

void CMapOutdoor::ApplyLight(DWORD dwVersion, const D3DLIGHT11& c_rkLight)
{
	STATEMANAGER.GetLight().SetLight(0, c_rkLight);
}

// 2004. 2. 17. myevan. 모든 부분을 보이게 초기화 한다
void CMapOutdoor::InitializeVisibleParts()
{
	m_dwVisiblePartFlags=0xffffffff;
}

// 2004. 2. 17. myevan. 특정 부분을 보이게 하거나 감추는 함수
void CMapOutdoor::SetVisiblePart(int ePart, bool isVisible)
{
	DWORD dwMask=(1<<ePart);
	if (isVisible)
	{
		m_dwVisiblePartFlags|=dwMask;
	}	
	else
	{
		DWORD dwReverseMask=~dwMask;
		m_dwVisiblePartFlags&=dwReverseMask;
	}
}

// 2004. 2. 17. myevan. 특정 부분이 보이는지 알아내는 함수
bool CMapOutdoor::IsVisiblePart(int ePart)
{
	DWORD dwMask=(1<<ePart);
	if (dwMask & m_dwVisiblePartFlags)
		return true;

	return false;
}

// Splat 개수 제한
void CMapOutdoor::SetSplatLimit(int iSplatNum)
{
	m_iSplatLimit = iSplatNum;
}

std::vector<int> & CMapOutdoor::GetRenderedSplatNum(int * piPatch, int * piSplat, float * pfSplatRatio)
{	
	*piPatch = m_iRenderedPatchNum;
	*piSplat = m_iRenderedSplatNum;
	*pfSplatRatio = m_iRenderedSplatNumSqSum/float(m_iRenderedPatchNum);

	return m_RenderedTextureNumVector;
}

CArea::TCRCWithNumberVector & CMapOutdoor::GetRenderedGraphicThingInstanceNum(DWORD * pdwGraphicThingInstanceNum, DWORD * pdwCRCNum)
{
	*pdwGraphicThingInstanceNum = m_dwRenderedGraphicThingInstanceNum;
	*pdwCRCNum = m_dwRenderedCRCNum;

	return m_dwRenderedCRCWithNumberVector;
}

void CMapOutdoor::RenderBeforeLensFlare()
{
	m_LensFlare.DrawBeforeFlare();	

	if (!mc_pEnvironmentData)
	{
		TraceError("CMapOutdoor::RenderBeforeLensFlare mc_pEnvironmentData is NULL");
		return;
	}
	
	m_LensFlare.Compute(mc_pEnvironmentData->DirLights[ENV_DIRLIGHT_BACKGROUND].Direction);
}

void CMapOutdoor::RenderAfterLensFlare()
{
	m_LensFlare.AdjustBrightness();
	m_LensFlare.DrawFlare();
}

void CMapOutdoor::RenderCollision()
{
	for (int i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea * pArea;
		if (GetAreaPointer(i, &pArea))
			pArea->RenderCollision();
	}
}

void CMapOutdoor::RenderScreenFiltering()
{
	m_ScreenFilter.Render();
}

void CMapOutdoor::RenderSky(const RenderContext& ctx)
{
	if (IsVisiblePart(PART_SKY))
		m_SkyBox.Render(ctx);
}

void CMapOutdoor::RenderCloud(const RenderContext& ctx)
{
	if (IsVisiblePart(PART_CLOUD))
		m_SkyBox.RenderCloud(ctx);
}

void CMapOutdoor::RenderTree(const RenderContext& ctx)
{
	if (IsVisiblePart(PART_TREE))
		CSpeedTreeForestDirectX::Instance().Render(ctx);
}

void CMapOutdoor::SetInverseViewAndDynamicShaodwMatrices()
{
	CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();
	if (!pCamera) return;

	m_matViewInverse = pCamera->GetInverseViewMatrix();

	XMFLOAT3 t = pCamera->GetTarget();
	XMFLOAT3 e(t.x - 1.732f * 1250.0f, t.y - 1250.0f, t.z + 2.0f * 1.732f * 1250.0f);
	XMFLOAT3 u(0, 0, 1);

	XMStoreFloat4x4(&m_matLightView,
		XMMatrixLookAtRH(XMLoadFloat3(&e), XMLoadFloat3(&t), XMLoadFloat3(&u)));

	XMStoreFloat4x4(&m_matDynamicShadow,
		XMMatrixMultiply(
			XMMatrixMultiply(XMLoadFloat4x4(&m_matViewInverse), XMLoadFloat4x4(&m_matLightView)),
			XMLoadFloat4x4(&m_matDynamicShadowScale)
		));
}

void CMapOutdoor::OnRender()
{
#ifdef __PERFORMANCE_CHECKER__
	DWORD t1=ELTimer_GetMSec();
	SetInverseViewAndDynamicShaodwMatrices();
	RenderFrameContext ctx = BuildRenderFrameContext();

	SetBlendOperation();
	RenderSky(ctx);
	RenderCloud(ctx);
	DWORD t2=ELTimer_GetMSec();
	RenderArea();
	DWORD t3=ELTimer_GetMSec();
	if (!m_bEnableTerrainOnlyForHeight)
		RenderTerrain(ctx);
	DWORD t4=ELTimer_GetMSec();
	RenderTree();
	DWORD t5=ELTimer_GetMSec();
	DWORD tEnd=ELTimer_GetMSec();

	if (tEnd-t1<7)
		return;

	static FILE* fp=fopen("perf_map_render.txt", "w");
 	fprintf(fp, "MAP.Total %d (Time %d)\n", tEnd-t1, ELTimer_GetMSec());
	fprintf(fp, "MAP.ENV %d\n", t2-t1);
	fprintf(fp, "MAP.OBJ %d\n", t3-t2);
	fprintf(fp, "MAP.TRN %d\n", t4-t3);
	fprintf(fp, "MAP.TRE %d\n", t5-t4);

#else
	SetInverseViewAndDynamicShaodwMatrices();
	RenderContext ctx = BuildRenderFrameContext();

	SetBlendOperation();


	if (!m_bEnableTerrainOnlyForHeight)
		RenderTerrain(ctx);
	RenderArea(ctx);
	RenderTree(ctx);

	RenderBlendArea(ctx);
#endif
}

struct FAreaRenderShadow
{
	const RenderContext& ctx;

	void operator()(CGraphicObjectInstance* pInstance)
	{
		if (!pInstance)
			return;

		pInstance->RenderShadow(ctx);

		if (!pInstance->IsObjectHeight())
			pInstance->Hide();
	}
};

struct FPCBlockerHide
{
	void operator () (CGraphicObjectInstance * pInstance)
	{
		pInstance->Hide();
	}
};

struct FRenderPCBlocker
{
	const RenderContext& ctx;

	void operator()(CGraphicObjectInstance* pInstance) const
	{
		if (!pInstance)
			return;

		pInstance->Show();

		CGraphicThingInstance* pThingInstance = dynamic_cast<CGraphicThingInstance*>(pInstance);
		if (pThingInstance && pThingInstance->HaveBlendThing())
		{
			pThingInstance->BlendRender(ctx);
			return;
		}

		pInstance->RenderPCBlocker(ctx);
	}
};

void CMapOutdoor::RenderEffect(const RenderContext& ctx)
{
	if (!IsVisiblePart(PART_OBJECT))
		return;
	for (int i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea * pArea;
		if (GetAreaPointer(i, &pArea))
		{
			pArea->RenderEffect(ctx);
		}
	}
}

struct CMapOutdoor_LessThingInstancePtrRenderOrder
{
	bool operator()(CGraphicThingInstance* pkLeft, CGraphicThingInstance* pkRight)
	{
		CCamera* pCurrentCamera = CCameraManager::Instance().GetCurrentCamera();
		const XMFLOAT3& cam = pCurrentCamera->GetEye();
		const XMFLOAT3& l = pkLeft->GetPosition();
		const XMFLOAT3& r = pkRight->GetPosition();

		XMVECTOR vCam = XMLoadFloat3(&cam);
		XMVECTOR vL = XMLoadFloat3(&l);
		XMVECTOR vR = XMLoadFloat3(&r);

		float dl = XMVectorGetX(XMVector3LengthSq(vCam - vL));
		float dr = XMVectorGetX(XMVector3LengthSq(vCam - vR));

		return dl < dr;
	}
};

struct CMapOutdoor_FOpaqueThingInstanceRender
{
	const RenderContext& ctx;

	inline void operator()(CGraphicThingInstance* pkThingInst) const
	{
		if (pkThingInst)
			pkThingInst->Render(ctx);
	}
};

struct CMapOutdoor_FBlendThingInstanceRender
{
	const RenderContext& ctx;

	inline void operator()(CGraphicThingInstance* pkThingInst) const
	{
		if (pkThingInst)
			pkThingInst->BlendRender(ctx);
	}
};

void CMapOutdoor::RenderArea(const RenderContext& ctx, bool bRenderAmbience)
{
	if (!IsVisiblePart(PART_OBJECT))
		return;

	m_dwRenderedCRCNum = 0;
	m_dwRenderedGraphicThingInstanceNum = 0;
	m_dwRenderedCRCWithNumberVector.clear();

	std::for_each(m_PCBlockerVector.begin(), m_PCBlockerVector.end(), FPCBlockerHide());

	if (ctx.Frame.DrawShadow && ctx.Frame.DrawCharacterShadow && ctx.Frame.CharacterShadowTexture)
	{
		_mgr->GetCbMgr()->SetFogColor(0xFFFFFFFF);

		STATEMANAGER.GetTransform().Push();
		STATEMANAGER.GetTransform().SetTexture1(ctx.Frame.DynamicShadowMatrix);

		STATEMANAGER.SetTexture(3, ctx.Frame.CharacterShadowTexture);

		STATEMANAGER.GetSampler().Push(3);
		STATEMANAGER.GetSampler().SetAddressUV(3, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
		STATEMANAGER.GetSampler().SetFilter(3, D3D11_FILTER_MIN_MAG_MIP_LINEAR);

		std::for_each(m_ShadowReceiverVector.begin(), m_ShadowReceiverVector.end(), FAreaRenderShadow(ctx));

		STATEMANAGER.GetSampler().Restore(3);
		STATEMANAGER.GetTransform().Restore();

		STATEMANAGER.SetTexture(3, NULL);
		_mgr->GetCbMgr()->SetFogColor(ctx.Frame.FogColor);
	}

	for (int j = 0; j < AROUND_AREA_NUM; ++j)
	{
		CArea* pArea;
		if (GetAreaPointer(j, &pArea))
			pArea->RenderDungeon(ctx);
	}

	STATEMANAGER.GetDepthStencil().Push();
	STATEMANAGER.GetDepthStencil().SetDepthEnable(TRUE);
	STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(TRUE);
	STATEMANAGER.GetDepthStencil().SetDepthFunc(D3D11_COMPARISON_LESS_EQUAL);

	static std::vector<CGraphicThingInstance*> s_kVct_pkOpaqueThingInstSort;
	s_kVct_pkOpaqueThingInstSort.clear();
	s_kVct_pkOpaqueThingInstSort.reserve(512);

	for (int i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea* pArea;
		if (GetAreaPointer(i, &pArea))
			pArea->CollectRenderingObject(s_kVct_pkOpaqueThingInstSort);
	}

	std::sort(s_kVct_pkOpaqueThingInstSort.begin(), s_kVct_pkOpaqueThingInstSort.end(), CMapOutdoor_LessThingInstancePtrRenderOrder());
	std::for_each(s_kVct_pkOpaqueThingInstSort.begin(), s_kVct_pkOpaqueThingInstSort.end(), CMapOutdoor_FOpaqueThingInstanceRender(ctx));

	STATEMANAGER.GetDepthStencil().Restore();

	if (m_bDrawShadow && m_bDrawChrShadow)
		std::for_each(m_ShadowReceiverVector.begin(), m_ShadowReceiverVector.end(), std::mem_fn(&CGraphicObjectInstance::Show));
}

void CMapOutdoor::RenderBlendArea(const RenderContext& ctx)
{
	if (!IsVisiblePart(PART_OBJECT))
		return;

	static std::vector<CGraphicThingInstance*> s_kVct_pkBlendThingInstSort;
	s_kVct_pkBlendThingInstSort.clear();
	s_kVct_pkBlendThingInstSort.reserve(256);  // Pre-allocate to avoid reallocations

	for (int i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea * pArea;
		if (GetAreaPointer(i, &pArea))
		{
			pArea->CollectBlendRenderingObject(s_kVct_pkBlendThingInstSort);
		}
	}

	if (s_kVct_pkBlendThingInstSort.size() != 0)
	{
		std::sort(s_kVct_pkBlendThingInstSort.begin(), s_kVct_pkBlendThingInstSort.end(), CMapOutdoor_LessThingInstancePtrRenderOrder());

		STATEMANAGER.GetStateCache().Push();

		STATEMANAGER.GetDepthStencil().SetDepthWriteEnable(true);
		STATEMANAGER.GetBlend().SetBlendEnable(true);
		STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
		STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

		std::for_each(s_kVct_pkBlendThingInstSort.begin(), s_kVct_pkBlendThingInstSort.end(), CMapOutdoor_FBlendThingInstanceRender(ctx));

		STATEMANAGER.GetStateCache().Restore();
	}
}
void CMapOutdoor::RenderDungeon(const RenderContext& ctx)
{
	for (int i = 0; i < AROUND_AREA_NUM; ++i)
	{
		CArea * pArea;
		if (!GetAreaPointer(i, &pArea))
			continue;
		pArea->RenderDungeon(ctx);
	}
}

void CMapOutdoor::RenderPCBlocker(const RenderContext& ctx)
{
	if (m_PCBlockerVector.size() != 0)
	{
		STATEMANAGER.SetTexture(0, NULL);

		STATEMANAGER.GetStateCache().Push();
		STATEMANAGER.GetSampler().Push(1);

		STATEMANAGER.GetBlend().SetBlendEnable(true);
		STATEMANAGER.GetSampler().SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

		STATEMANAGER.GetTransform().Push();
		STATEMANAGER.GetTransform().SetTexture1(m_matBuildingTransparent);
		STATEMANAGER.SetTexture(1, m_BuildingTransparentImageInstance.GetTexturePointer()->GetSRV());

		std::for_each(m_PCBlockerVector.begin(), m_PCBlockerVector.end(), FRenderPCBlocker(ctx));

		STATEMANAGER.SetTexture(1, NULL);
		STATEMANAGER.GetTransform().Restore();

		STATEMANAGER.GetSampler().Restore(1);
		STATEMANAGER.GetStateCache().Restore();
	}
}

void CMapOutdoor::SelectIndexBuffer(BYTE byLODLevel, WORD * pwPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY * pePrimitiveType)
{
	if (0 == byLODLevel)
	{
		*pwPrimitiveCount = m_wNumIndices[byLODLevel] - 2;
		*pePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP;
	}
	else
	{
		*pwPrimitiveCount =  m_wNumIndices[byLODLevel]/3;
		*pePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	}
	_mgr->SetIndexBuffer(m_IndexBuffer[byLODLevel]);
}

void CMapOutdoor::SetPatchDrawVector()
{
	assert(NULL!=m_pTerrainPatchProxyList && "CMapOutdoor::__SetPatchDrawVector");

	m_PatchDrawStructVector.clear();

	std::vector<std::pair<float, long> >::iterator aDistancePatchVectorIterator;

	TPatchDrawStruct aPatchDrawStruct;

	aDistancePatchVectorIterator = m_PatchVector.begin();
	while(aDistancePatchVectorIterator != m_PatchVector.end())
	{
		std::pair<float, long> adistancePatchPair = *aDistancePatchVectorIterator;

		CTerrainPatchProxy * pTerrainPatchProxy = &m_pTerrainPatchProxyList[adistancePatchPair.second];

		if (!pTerrainPatchProxy->isUsed())
		{
			++aDistancePatchVectorIterator;
			continue;
		}

		long lPatchNum = pTerrainPatchProxy->GetPatchNum();
		if (lPatchNum < 0)
		{
			++aDistancePatchVectorIterator;
			continue;
		}

		BYTE byTerrainNum = pTerrainPatchProxy->GetTerrainNum();
		if (0xFF == byTerrainNum)
		{
			++aDistancePatchVectorIterator;
			continue;
		}

		CTerrain * pTerrain;
		if (!GetTerrainPointer(byTerrainNum, &pTerrain))
		{
			++aDistancePatchVectorIterator;
			continue;
		}

		aPatchDrawStruct.fDistance				= adistancePatchPair.first;
		aPatchDrawStruct.byTerrainNum			= byTerrainNum;
		aPatchDrawStruct.lPatchNum				= lPatchNum;
		aPatchDrawStruct.pTerrainPatchProxy		= pTerrainPatchProxy;

		m_PatchDrawStructVector.push_back(aPatchDrawStruct);

		++aDistancePatchVectorIterator;
	}

	std::stable_sort(m_PatchDrawStructVector.begin(), m_PatchDrawStructVector.end(), FSortPatchDrawStructWithTerrainNum());
}

float CMapOutdoor::__GetNoFogDistance()
{
	return (float)(CTerrainImpl::CELLSCALE * m_lViewRadius) * 0.5f;
}

float CMapOutdoor::__GetFogDistance()
{
	return (float)(CTerrainImpl::CELLSCALE * m_lViewRadius) * 0.75f;
}

struct FPatchNumMatch
{
	long m_lPatchNumToCheck;
	FPatchNumMatch(long lPatchNum)
	{
		m_lPatchNumToCheck = lPatchNum;
	}
	bool operator() (std::pair<long, BYTE> aPair)
	{
		return m_lPatchNumToCheck == aPair.first;
	}
};

void CMapOutdoor::NEW_DrawWireFrame(CTerrainPatchProxy* pTerrainPatchProxy, WORD wPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType)
{
	STATEMANAGER.GetRaster().Push();

	STATEMANAGER.GetRaster().SetFillMode(D3D11_FILL_WIREFRAME);
	_mgr->GetCbMgr()->SetFogEnable(false);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);

	STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);

	STATEMANAGER.GetRaster().Restore();
}

void CMapOutdoor::DrawWireFrame(long patchnum, WORD wPrimitiveCount, D3D11_PRIMITIVE_TOPOLOGY ePrimitiveType)
{
	assert(NULL != m_pTerrainPatchProxyList && "CMapOutdoor::DrawWireFrame");

	CTerrainPatchProxy* pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];

	if (!pTerrainPatchProxy->isUsed())
		return;

	long sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	BYTE ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	STATEMANAGER.GetRaster().Push();

	STATEMANAGER.GetRaster().SetFillMode(D3D11_FILL_WIREFRAME);
	_mgr->GetCbMgr()->SetFogEnable(false);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.SetTexture(1, NULL);

	STATEMANAGER.DrawIndexedPrimitive11(ePrimitiveType, 0, 0, wPrimitiveCount);

	STATEMANAGER.GetRaster().Restore();
}

// Attr
void CMapOutdoor::RenderMarkedArea()
{
	if (!m_pTerrainPatchProxyList)
		return;

	m_matWorldForCommonUse._41 = 0.0f;
	m_matWorldForCommonUse._42 = 0.0f;

	STATEMANAGER.GetTransform().SetWorld(m_matWorldForCommonUse);

	WORD wPrimitiveCount;
	D3D11_PRIMITIVE_TOPOLOGY eType;
	SelectIndexBuffer(0, &wPrimitiveCount, &eType);

	XMFLOAT4X4 matTexTransform;
	XMStoreFloat4x4(&matTexTransform, XMMatrixMultiply(XMLoadFloat4x4(&m_matViewInverse), XMMatrixScaling(m_fTerrainTexCoordBase * 32.0f, -m_fTerrainTexCoordBase * 32.0f, 0.0f)));

	STATEMANAGER.GetTransform().Push();
	STATEMANAGER.GetTransform().SetTexture0(matTexTransform);
	STATEMANAGER.GetTransform().SetTexture1(matTexTransform);

	STATEMANAGER.GetStateCache().Push();
	STATEMANAGER.GetSampler().Push(1);

	STATEMANAGER.GetBlend().SetBlendEnable(true);
	STATEMANAGER.GetBlend().SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	STATEMANAGER.GetBlend().SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	static long lStartTime = timeGetTime();
	float fTime = float((timeGetTime() - lStartTime) % 3000) / 3000.0f;
	float fAlpha = fabs(fTime - 0.5f) / 2.0f + 0.1f;

	_mgr->GetCbMgr()->SetTextureFactor(ColorToUint(XMFLOAT4(1.0f, 1.0f, 1.0f, fAlpha)));

	STATEMANAGER.GetSampler().SetFilter(1, D3D11_FILTER_MIN_MAG_MIP_POINT);
	STATEMANAGER.GetSampler().SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	STATEMANAGER.SetTexture(0, m_attrImageInstance.GetTexturePointer()->GetSRV());

	_mgr->SetShader(VF_TERRAIN);

	RecurseRenderAttr(m_pRootNode);

	STATEMANAGER.GetSampler().Restore(1);
	STATEMANAGER.GetStateCache().Restore();

	STATEMANAGER.GetTransform().Restore();
}

void CMapOutdoor::RecurseRenderAttr(CTerrainQuadtreeNode *Node, bool bCullEnable)
{
	if (bCullEnable)
	{
		if (__RenderTerrain_RecurseRenderQuadTree_CheckBoundingCircle(Node->center, Node->radius)==VIEW_NONE)
			return;
	}

	{
		if (Node->Size == 1)
		{
			DrawPatchAttr(Node->PatchNum);
		}
		else
		{
			if (Node->NW_Node != NULL)
				RecurseRenderAttr(Node->NW_Node, bCullEnable);
			if (Node->NE_Node != NULL)
				RecurseRenderAttr(Node->NE_Node, bCullEnable);
			if (Node->SW_Node != NULL)
				RecurseRenderAttr(Node->SW_Node, bCullEnable);
			if (Node->SE_Node != NULL)
				RecurseRenderAttr(Node->SE_Node, bCullEnable);
		}
 	}
}

void CMapOutdoor::DrawPatchAttr(long patchnum)
{
	CTerrainPatchProxy * pTerrainPatchProxy = &m_pTerrainPatchProxyList[patchnum];
	if (!pTerrainPatchProxy->isUsed())
		return;

	long sPatchNum = pTerrainPatchProxy->GetPatchNum();
	if (sPatchNum < 0)
		return;

	BYTE ucTerrainNum = pTerrainPatchProxy->GetTerrainNum();
	if (0xFF == ucTerrainNum)
		return;

	// Deal with this material buffer
	CTerrain * pTerrain;
	if (!GetTerrainPointer(ucTerrainNum, &pTerrain))
		return;

	if (!pTerrain->IsMarked())
		return;

	WORD wCoordX, wCoordY;
	pTerrain->GetCoordinate(&wCoordX, &wCoordY);

	m_matWorldForCommonUse._41 = -(float) (wCoordX * CTerrainImpl::XSIZE * CTerrainImpl::CELLSCALE);
	m_matWorldForCommonUse._42 = (float) (wCoordY * CTerrainImpl::YSIZE * CTerrainImpl::CELLSCALE);

	XMFLOAT4X4 matTexTransform;

	XMStoreFloat4x4(&matTexTransform,
		XMMatrixMultiply(
			XMMatrixMultiply(XMLoadFloat4x4(&m_matViewInverse), XMLoadFloat4x4(&m_matWorldForCommonUse)),
			XMLoadFloat4x4(&m_matStaticShadow)));

	STATEMANAGER.GetTransform().SetTexture1(matTexTransform);

	TTerrainSplatPatch & rAttrSplatPatch = pTerrain->GetMarkedSplatPatch();
 	STATEMANAGER.SetTexture(1, rAttrSplatPatch.Splats[0].pd3dTexture);

	_mgr->SetShader(VF_TERRAIN);

	_mgr->SetVertexBuffer(pTerrainPatchProxy->HardwareTransformPatch_GetVertexBufferPtr());
	STATEMANAGER.DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP, 0, 0, m_wNumIndices[0] - 2);
}
