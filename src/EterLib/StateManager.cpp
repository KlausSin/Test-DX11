#include "StdAfx.h"
#include "StateManager.h"
#include "GrpLightManager.h"
#include "GrpD3D11Renderer.h"
#include "qMin32Lib/ConstantBufferManager.h"

static int g_frameDrawCount = 0;
static int g_frameNumber = 0;
static int g_drawByStride[64] = {};

#define StateManager_Assert(a) assert(a)

void CStateManager::SetScissorRect(const RECT& c_rRect)
{
	if (CGraphicBase::ms_lpd3d11Context)
	{
		D3D11_RECT r = { c_rRect.left, c_rRect.top, c_rRect.right, c_rRect.bottom };
		CGraphicBase::ms_lpd3d11Context->RSSetScissorRects(1, &r);
	}
}

void CStateManager::GetScissorRect(RECT* pRect)
{
	if (CGraphicBase::ms_lpd3d11Context)
	{
		UINT n = 1;
		D3D11_RECT r = {};
		CGraphicBase::ms_lpd3d11Context->RSGetScissorRects(&n, &r);
		pRect->left = r.left; pRect->top = r.top; pRect->right = r.right; pRect->bottom = r.bottom;
	}
}

bool CStateManager::BeginScene()
{
	m_bScene = true;

	const D3DXMATRIX& m4World = STATEMANAGER.GetTransform().GetWorld();
	const D3DXMATRIX& m4Proj = STATEMANAGER.GetTransform().GetProjection();
	const D3DXMATRIX& m4View = STATEMANAGER.GetTransform().GetView();

	STATEMANAGER.GetTransform().SetWorld(m4World);
	STATEMANAGER.GetTransform().SetProjection(m4Proj);
	STATEMANAGER.GetTransform().SetView(m4View);

	static int s_beginCount = 0;
	if ((s_beginCount++ % 60) == 0)
	{
		UINT n = 1;
		D3D11_VIEWPORT vp = {};
		if (CGraphicBase::ms_lpd3d11Context)
			CGraphicBase::ms_lpd3d11Context->RSGetViewports(&n, &vp);
		Tracenf("BeginScene: viewport=(%g,%g %gx%g)", vp.TopLeftX, vp.TopLeftY, vp.Width, vp.Height);
	}

	if (CGraphicBase::ms_lpd3d11Context && CGraphicBase::ms_lpd3d11RTV)
	{
		CGraphicBase::ms_lpd3d11Context->OMSetRenderTargets(1, &CGraphicBase::ms_lpd3d11RTV, CGraphicBase::ms_lpd3d11DSV);

		D3D11_VIEWPORT vp = {};
		vp.TopLeftX = 0;
		vp.TopLeftY = 0;
		vp.Width = (float)CGraphicBase::ms_iWidth;
		vp.Height = (float)CGraphicBase::ms_iHeight;
		vp.MinDepth = 0.0f;
		vp.MaxDepth = 1.0f;
		CGraphicBase::ms_lpd3d11Context->RSSetViewports(1, &vp);

		float clearColor[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
		CGraphicBase::ms_lpd3d11Context->ClearRenderTargetView(CGraphicBase::ms_lpd3d11RTV, clearColor);
		if (CGraphicBase::ms_lpd3d11DSV)
			CGraphicBase::ms_lpd3d11Context->ClearDepthStencilView(CGraphicBase::ms_lpd3d11DSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	}
	return true;
}

void CStateManager::EndScene()
{
	m_bScene = false;
	++g_frameNumber;
	if ((g_frameNumber % 120) == 0)
	{
		Tracenf("D3D11: Frame %d - draw calls = %d", g_frameNumber, g_frameDrawCount);
		Tracenf("  draws by stride: 16=%d 20=%d 24=%d 28=%d 32=%d 36=%d 40=%d",
			g_drawByStride[4], g_drawByStride[5], g_drawByStride[6], g_drawByStride[7],
			g_drawByStride[8], g_drawByStride[9], g_drawByStride[10]);
	}
	g_frameDrawCount = 0;
	for (int i = 0; i < 64; ++i) g_drawByStride[i] = 0;
}

CStateManager::CStateManager() : m_pD3D11Renderer(NULL)
{
	m_bScene = false;

#ifdef _DEBUG
	m_iDrawCallCount = 0;
	m_iLastDrawCallCount = 0;
#endif
}

CStateManager::~CStateManager()
{
}

void CStateManager::SetBestFiltering(DWORD dwStage)
{
	GetStateCache().Sampler.SetFilter(dwStage, D3D11_FILTER_ANISOTROPIC);
	GetStateCache().Sampler.SetMaxAnisotropy(dwStage, 8);
}

void CStateManager::Restore()
{
	int i, j;

	m_bForce = true;

	for (i = 0; i < STATEMANAGER_MAX_STAGES; ++i)
	{
		SetTexture(i, m_CurrentState.m_Textures[i]);
	}

	m_bForce = false;
}

void CStateManager::SetDefaultState()
{
	m_CurrentState.ResetState();
	m_CurrentState_Copy.ResetState();

	for (auto& stack : m_TextureStack)
		stack.clear();

	for (auto& stack : m_StreamStack)
		stack.clear();

	m_IndexStack.clear();

	m_bScene = false;
	m_bForce = true;

	auto& state = GetStateCache();
	auto cb = _mgr->GetCbMgr();

	state.ResetDefault();
	state.ClearStacks();
	state.ForceDirty();

	D3DXMATRIX matIdentity;
	D3DXMatrixIdentity(&matIdentity);

	GetStateCache().Transform.SetWorld(matIdentity);
	GetStateCache().Transform.SetView(matIdentity);
	GetStateCache().Transform.SetProjection(matIdentity);

	D3DMATERIAL11 DefaultMat;
	ZeroMemory(&DefaultMat, sizeof(D3DMATERIAL11));

	DefaultMat.Diffuse.r = 1.0f;
	DefaultMat.Diffuse.g = 1.0f;
	DefaultMat.Diffuse.b = 1.0f;
	DefaultMat.Diffuse.a = 1.0f;

	DefaultMat.Ambient.r = 1.0f;
	DefaultMat.Ambient.g = 1.0f;
	DefaultMat.Ambient.b = 1.0f;
	DefaultMat.Ambient.a = 1.0f;

	STATEMANAGER.GetLight().SetMaterial(DefaultMat);

	cb->SetAlphaRef(1);
	cb->SetFogStart(0.0f);
	cb->SetFogEnd(0.0f);
	cb->SetAmbient(0x00000000);
	cb->SetFogEnable(false);
	cb->SetFogColor(0xFF000000);
	cb->SetAlphaTestEnable(false);
	cb->SetLightingEnable(false);

	state.Raster.SetScissorEnable(false);
	state.Raster.SetFillMode(D3D11_FILL_SOLID);
	state.Raster.SetCullMode(D3D11_CULL_FRONT);

	state.Blend.SetColorWriteMask(0x0F);
	state.Blend.SetBlendEnable(false);
	state.Blend.SetBlendOp(D3D11_BLEND_OP_ADD);
	state.Blend.SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	state.Blend.SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);

	state.DepthStencil.SetDepthEnable(true);
	state.DepthStencil.SetDepthFunc(D3D11_COMPARISON_LESS_EQUAL);
	state.DepthStencil.SetDepthWriteEnable(true);
	state.DepthStencil.SetStencilEnable(false);

	for (DWORD i = 0; i < 8; ++i)
	{
		state.Sampler.SetFilter(i, D3D11_FILTER_MIN_MAG_MIP_LINEAR);
		state.Sampler.SetAddressUVW(i, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);
		SetTexture(i, NULL);
	}

	m_bForce = false;
}

void CStateManager::StateManager_Capture()
{
	m_CurrentState_Copy = m_CurrentState;
}

void CStateManager::StateManager_Apply()
{
	m_CurrentState = m_CurrentState_Copy;
}

void CStateManager::SetD3D11Renderer(CD3D11Renderer* pRenderer)
{
	m_pD3D11Renderer = pRenderer;

	if (m_pD3D11Renderer)
		SetDefaultState();
}

CD3D11StateCacheSet& CStateManager::GetStateCache()
{
	assert(m_pD3D11Renderer);
	return m_pD3D11Renderer->GetStateCache();
}

CD3D11SamplerStateCache& CStateManager::GetSampler()
{
	return GetStateCache().Sampler;
}

CD3D11RasterStateCache& CStateManager::GetRaster()
{
	return GetStateCache().Raster;
}

CD3D11DepthStencilStateCache& CStateManager::GetDepthStencil()
{
	return GetStateCache().DepthStencil;
}

CD3D11BlendStateCache& CStateManager::GetBlend()
{
	return GetStateCache().Blend;
}

CD3D11TransformStateCache& CStateManager::GetTransform()
{
	return GetStateCache().Transform;
}

CD3D11LightingStateCache& CStateManager::GetLight()
{
	return GetStateCache().Light;
}

// Textures (D3D11 SRV)
void CStateManager::SaveTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV)
{
	m_TextureStack[dwStage].push_back(m_CurrentState.m_Textures[dwStage]);
	SetTexture(dwStage, pSRV);
}

void CStateManager::RestoreTexture(DWORD dwStage)
{
	SetTexture(dwStage, m_TextureStack[dwStage].back());
	m_TextureStack[dwStage].pop_back();
}

void CStateManager::SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV)
{
	// No cache check — D3D11 SRV pointers may be reused after destroy/recreate
	// causing stale bindings to silently fail
	m_CurrentState.m_Textures[dwStage] = pSRV;

	if (m_pD3D11Renderer)
		m_pD3D11Renderer->SetTexture(dwStage, pSRV);
}

void CStateManager::GetTexture(DWORD dwStage, ID3D11ShaderResourceView** ppSRV)
{
	*ppSRV = m_CurrentState.m_Textures[dwStage];
}

void CStateManager::SaveStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride)
{
	m_StreamStack[StreamNumber].push_back(m_CurrentState.m_StreamData[StreamNumber]);
	SetStreamSource(StreamNumber, pStreamData, Stride);
}

void CStateManager::RestoreStreamSource(UINT StreamNumber)
{
	const auto& topStream = m_StreamStack[StreamNumber].back();
	SetStreamSource(StreamNumber, topStream.pBuffer, topStream.stride);
	m_StreamStack[StreamNumber].pop_back();
}

void CStateManager::SetStreamSource(UINT StreamNumber, ID3D11Buffer* pStreamData, UINT Stride)
{
	auto& cur = m_CurrentState.m_StreamData[StreamNumber];

	if (!m_bForce && cur.pBuffer == pStreamData && cur.stride == Stride)
		return;

	cur.pBuffer = pStreamData;
	cur.stride = Stride;

	if (CGraphicBase::ms_lpd3d11Context)
	{
		UINT uStride = Stride;
		UINT uOffset = 0;
		CGraphicBase::ms_lpd3d11Context->IASetVertexBuffers(StreamNumber, 1, &pStreamData, &uStride, &uOffset);
	}
}

void CStateManager::SaveIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex)
{
	m_IndexStack.push_back(m_CurrentState.m_IndexData);
	SetIndices(pIndexData, BaseVertexIndex);
}

void CStateManager::RestoreIndices()
{
	const auto& topIndex = m_IndexStack.back();
	SetIndices(topIndex.pBuffer, topIndex.baseVertexIndex);
	m_IndexStack.pop_back();
}

void CStateManager::SetIndices(ID3D11Buffer* pIndexData, UINT BaseVertexIndex)
{
	auto& cur = m_CurrentState.m_IndexData;

	if (!m_bForce && cur.pBuffer == pIndexData && cur.baseVertexIndex == BaseVertexIndex)
		return;

	cur.pBuffer = pIndexData;
	cur.baseVertexIndex = BaseVertexIndex;

	if (CGraphicBase::ms_lpd3d11Context)
		CGraphicBase::ms_lpd3d11Context->IASetIndexBuffer(pIndexData, DXGI_FORMAT_R16_UINT, 0);
}

static UINT GetPrimitiveVertexCount(D3D11_PRIMITIVE_TOPOLOGY topology, UINT primCount)
{
	switch (topology)
	{
	case D3D11_PRIMITIVE_TOPOLOGY_POINTLIST:		return primCount;
	case D3D11_PRIMITIVE_TOPOLOGY_LINELIST:			return primCount * 2;
	case D3D11_PRIMITIVE_TOPOLOGY_LINESTRIP:		return primCount + 1;
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST:		return primCount * 3;
	case D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP:	return primCount + 2;
	default:										return primCount * 3;
	}
}

HRESULT CStateManager::DrawPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, UINT PrimitiveCount, UINT StartVertex_VertexStride, const void* pVertexData)
{
#ifdef _DEBUG
	++m_iDrawCallCount;
#endif

	if (!CGraphicBase::ms_lpd3d11Context || !m_pD3D11Renderer)
		return E_FAIL;

	UINT vertexCount = GetPrimitiveVertexCount(Topology, PrimitiveCount);

	ID3D11Buffer* vb = nullptr;

	if (pVertexData)
	{
		//StartVertex_VertexStride = stride
		UINT stride = StartVertex_VertexStride;

		if (!CGraphicBase::ms_lpd3d11Device || !stride)
			return E_FAIL;

		D3D11_BUFFER_DESC bd = {};
		bd.ByteWidth = vertexCount * stride;
		bd.Usage = D3D11_USAGE_IMMUTABLE;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

		D3D11_SUBRESOURCE_DATA sd = {};
		sd.pSysMem = pVertexData;

		if (FAILED(CGraphicBase::ms_lpd3d11Device->CreateBuffer(&bd, &sd, &vb)))
			return E_FAIL;

		UINT offset = 0;
		CGraphicBase::ms_lpd3d11Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

		StartVertex_VertexStride = 0; // = startVertex
	}

	m_pD3D11Renderer->FlushAllState();
	CGraphicBase::ms_lpd3d11Context->IASetPrimitiveTopology(Topology);
	CGraphicBase::ms_lpd3d11Context->Draw(vertexCount, StartVertex_VertexStride);

	if (vb)
		vb->Release();

	return S_OK;
}

HRESULT CStateManager::DrawIndexedPrimitive11(D3D11_PRIMITIVE_TOPOLOGY Topology, INT BaseVertexIndex, UINT StartIndex, UINT PrimitiveCount)
{
#ifdef _DEBUG
	++m_iDrawCallCount;
#endif

	++g_frameDrawCount;

	UINT stride = m_CurrentState.m_StreamData[0].stride;
	if ((stride / 4) < 64)
		g_drawByStride[stride / 4]++;

	if (!CGraphicBase::ms_lpd3d11Context || !m_pD3D11Renderer)
		return E_FAIL;

	m_pD3D11Renderer->FlushAllState();
	CGraphicBase::ms_lpd3d11Context->IASetPrimitiveTopology(Topology);
	CGraphicBase::ms_lpd3d11Context->DrawIndexed(GetPrimitiveVertexCount(Topology, PrimitiveCount), StartIndex, BaseVertexIndex);

	return S_OK;
}

HRESULT CStateManager::DrawTriangleFan11(UINT PrimitiveCount, const void* pVertexData, UINT VertexStride)
{
#ifdef _DEBUG
	++m_iDrawCallCount;
#endif

	if (!CGraphicBase::ms_lpd3d11Context || !CGraphicBase::ms_lpd3d11Device || !m_pD3D11Renderer)
		return E_FAIL;

	if (!pVertexData || !VertexStride || !PrimitiveCount)
		return E_FAIL;

	UINT finalVertexCount = PrimitiveCount * 3;
	std::vector<BYTE> data(finalVertexCount * VertexStride);

	const BYTE* src = (const BYTE*)pVertexData;
	BYTE* dst = data.data();

	for (UINT i = 0; i < PrimitiveCount; ++i)
	{
		memcpy(dst, src, VertexStride);
		dst += VertexStride;
		memcpy(dst, src + (i + 1) * VertexStride, VertexStride);
		dst += VertexStride;
		memcpy(dst, src + (i + 2) * VertexStride, VertexStride);
		dst += VertexStride;
	}

	D3D11_BUFFER_DESC bd = {};
	bd.ByteWidth = finalVertexCount * VertexStride;
	bd.Usage = D3D11_USAGE_IMMUTABLE;
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA sd = {};
	sd.pSysMem = data.data();

	ID3D11Buffer* vb = nullptr;
	if (FAILED(CGraphicBase::ms_lpd3d11Device->CreateBuffer(&bd, &sd, &vb)))
		return E_FAIL;

	UINT stride = VertexStride;
	UINT offset = 0;

	CGraphicBase::ms_lpd3d11Context->IASetVertexBuffers(0, 1, &vb, &stride, &offset);

	m_pD3D11Renderer->FlushAllState();
	CGraphicBase::ms_lpd3d11Context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	CGraphicBase::ms_lpd3d11Context->Draw(finalVertexCount, 0);

	vb->Release();
	return S_OK;
}

#ifdef _DEBUG
void CStateManager::ResetDrawCallCounter()
{
	m_iLastDrawCallCount = m_iDrawCallCount;
	m_iDrawCallCount = 0;
}

int CStateManager::GetDrawCallCount() const
{
	return m_iLastDrawCallCount;
}
#endif