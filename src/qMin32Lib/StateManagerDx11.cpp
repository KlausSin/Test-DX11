
#include "pch.h"
#include "StateManagerDx11.h"

CStateManagerDx11::CStateManagerDx11(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice(pDevice), m_pContext(pContext)
{
	memset(this, 0, sizeof(*this));

	for (UINT i = 0; i < 8; ++i)
		m_samplerFilters[i] = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	m_topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	m_blendKey.enable = FALSE;
	m_blendKey.src = D3D11_BLEND_SRC_ALPHA;
	m_blendKey.dst = D3D11_BLEND_INV_SRC_ALPHA;
	m_blendKey.op = D3D11_BLEND_OP_ADD;

	m_depthKey.enable = TRUE;
	m_depthKey.write = TRUE;
	m_depthKey.func = D3D11_COMPARISON_LESS_EQUAL;

	m_rasterKey.fillMode = D3D11_FILL_SOLID;
	m_rasterKey.cullMode = D3D11_CULL_BACK;
	m_rasterKey.scissor = FALSE;
}

CStateManagerDx11::~CStateManagerDx11()
{
	for (auto& it : m_samplerCache)
		if (it.second)
			it.second->Release();

	for (auto& it : m_blendCache)
		if (it.second)
			it.second->Release();

	for (auto& it : m_depthCache)
		if (it.second)
			it.second->Release();

	for (auto& it : m_rasterCache)
		if (it.second)
			it.second->Release();

	m_samplerCache.clear();
	m_blendCache.clear();
	m_depthCache.clear();
	m_rasterCache.clear();
}

ID3D11SamplerState* CStateManagerDx11::GetOrCreateSampler(const SSamplerKey& key)
{
	auto it = m_samplerCache.find(key);

	if (it != m_samplerCache.end())
		return it->second;

	D3D11_SAMPLER_DESC sd = {};
	sd.Filter = key.filter;
	sd.AddressU = key.addressU;
	sd.AddressV = key.addressV;
	sd.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	sd.MaxLOD = D3D11_FLOAT32_MAX;
	sd.MaxAnisotropy = key.filter == D3D11_FILTER_ANISOTROPIC ? 8 : 1;

	ID3D11SamplerState* pState = nullptr;

	if (FAILED(m_pDevice->CreateSamplerState(&sd, &pState)))
		return nullptr;

	m_samplerCache[key] = pState;
	return pState;
}

void CStateManagerDx11::SetSamplerFilter(UINT stage, D3D11_FILTER filter)
{
	m_samplerFilters[stage] = filter;
}

void CStateManagerDx11::SetSamplerWrap(UINT stage)
{
	SSamplerKey key;
	key.filter = m_samplerFilters[stage];
	key.addressU = D3D11_TEXTURE_ADDRESS_WRAP;
	key.addressV = D3D11_TEXTURE_ADDRESS_WRAP;

	ID3D11SamplerState* pState = GetOrCreateSampler(key);

	if (m_pSamplers[stage] == pState)
		return;

	m_pContext->PSSetSamplers(stage, 1, &pState);
	m_pSamplers[stage] = pState;
}

void CStateManagerDx11::SetSamplerClamp(UINT stage)
{
	SSamplerKey key;
	key.filter = m_samplerFilters[stage];
	key.addressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	key.addressV = D3D11_TEXTURE_ADDRESS_CLAMP;

	ID3D11SamplerState* pState = GetOrCreateSampler(key);

	if (m_pSamplers[stage] == pState)
		return;

	m_pContext->PSSetSamplers(stage, 1, &pState);
	m_pSamplers[stage] = pState;
}

void CStateManagerDx11::SetTexture(UINT stage, ID3D11ShaderResourceView* pSRV)
{
	if (m_pTextures[stage] == pSRV)
		return;

	m_pContext->PSSetShaderResources(stage, 1, &pSRV);
	m_pTextures[stage] = pSRV;
}

void CStateManagerDx11::SetVertexBuffer(UINT slot, ID3D11Buffer* pVB, UINT stride, UINT offset)
{
	if (m_pVB[slot] == pVB &&
		m_vertexStride[slot] == stride &&
		m_vertexOffset[slot] == offset)
		return;

	m_pContext->IASetVertexBuffers(slot, 1, &pVB, &stride, &offset);

	m_pVB[slot] = pVB;
	m_vertexStride[slot] = stride;
	m_vertexOffset[slot] = offset;
}

void CStateManagerDx11::SetIndexBuffer(ID3D11Buffer* pIB, DXGI_FORMAT format, UINT offset)
{
	if (m_pIB == pIB &&
		m_ibFormat == format &&
		m_ibOffset == offset)
		return;

	m_pContext->IASetIndexBuffer(pIB, format, offset);

	m_pIB = pIB;
	m_ibFormat = format;
	m_ibOffset = offset;
}

void CStateManagerDx11::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	if (m_topology == topology)
		return;

	m_pContext->IASetPrimitiveTopology(topology);
	m_topology = topology;
}

ID3D11BlendState* CStateManagerDx11::GetOrCreateBlend(const SBlendKey& key)
{
	auto it = m_blendCache.find(key);

	if (it != m_blendCache.end())
		return it->second;

	D3D11_BLEND_DESC bd = {};

	bd.RenderTarget[0].BlendEnable = key.enable;
	bd.RenderTarget[0].SrcBlend = key.src;
	bd.RenderTarget[0].DestBlend = key.dst;
	bd.RenderTarget[0].BlendOp = key.op;
	bd.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	bd.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
	bd.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	bd.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* pState = nullptr;

	if (FAILED(m_pDevice->CreateBlendState(&bd, &pState)))
		return nullptr;

	m_blendCache[key] = pState;
	return pState;
}

void CStateManagerDx11::SetAlphaBlend(BOOL enable)
{
	if (m_blendKey.enable == enable)
		return;

	m_blendKey.enable = enable;

	ID3D11BlendState* pState = GetOrCreateBlend(m_blendKey);

	float blendFactor[4] = { 0,0,0,0 };

	m_pContext->OMSetBlendState(pState, blendFactor, 0xffffffff);
}

void CStateManagerDx11::SetBlendFunc(D3D11_BLEND src, D3D11_BLEND dst)
{
	m_blendKey.src = src;
	m_blendKey.dst = dst;

	ID3D11BlendState* pState = GetOrCreateBlend(m_blendKey);

	float blendFactor[4] = { 0,0,0,0 };

	m_pContext->OMSetBlendState(pState, blendFactor, 0xffffffff);
}

void CStateManagerDx11::SetBlendOp(D3D11_BLEND_OP op)
{
	m_blendKey.op = op;

	ID3D11BlendState* pState = GetOrCreateBlend(m_blendKey);

	float blendFactor[4] = { 0,0,0,0 };

	m_pContext->OMSetBlendState(pState, blendFactor, 0xffffffff);
}

ID3D11DepthStencilState* CStateManagerDx11::GetOrCreateDepth(const SDepthKey& key)
{
	auto it = m_depthCache.find(key);

	if (it != m_depthCache.end())
		return it->second;

	D3D11_DEPTH_STENCIL_DESC dd = {};

	dd.DepthEnable = key.enable;
	dd.DepthWriteMask = key.write ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO;
	dd.DepthFunc = key.func;

	ID3D11DepthStencilState* pState = nullptr;

	if (FAILED(m_pDevice->CreateDepthStencilState(&dd, &pState)))
		return nullptr;

	m_depthCache[key] = pState;
	return pState;
}

void CStateManagerDx11::SetDepthEnable(BOOL enable)
{
	if (m_depthKey.enable == enable)
		return;

	m_depthKey.enable = enable;

	ID3D11DepthStencilState* pState = GetOrCreateDepth(m_depthKey);
	m_pContext->OMSetDepthStencilState(pState, 0);
}

void CStateManagerDx11::SetDepthWrite(BOOL enable)
{
	if (m_depthKey.write == enable)
		return;

	m_depthKey.write = enable;

	ID3D11DepthStencilState* pState = GetOrCreateDepth(m_depthKey);
	m_pContext->OMSetDepthStencilState(pState, 0);
}

void CStateManagerDx11::SetDepthFunc(D3D11_COMPARISON_FUNC func)
{
	if (m_depthKey.func == func)
		return;

	m_depthKey.func = func;

	ID3D11DepthStencilState* pState = GetOrCreateDepth(m_depthKey);
	m_pContext->OMSetDepthStencilState(pState, 0);
}

ID3D11RasterizerState* CStateManagerDx11::GetOrCreateRaster(const SRasterKey& key)
{
	auto it = m_rasterCache.find(key);

	if (it != m_rasterCache.end())
		return it->second;

	D3D11_RASTERIZER_DESC rd = {};

	rd.FillMode = key.fillMode;
	rd.CullMode = key.cullMode;
	rd.ScissorEnable = key.scissor;
	rd.DepthClipEnable = TRUE;

	ID3D11RasterizerState* pState = nullptr;

	if (FAILED(m_pDevice->CreateRasterizerState(&rd, &pState)))
		return nullptr;

	m_rasterCache[key] = pState;
	return pState;
}

void CStateManagerDx11::SetCullMode(D3D11_CULL_MODE mode)
{
	if (m_rasterKey.cullMode == mode)
		return;

	m_rasterKey.cullMode = mode;

	ID3D11RasterizerState* pState = GetOrCreateRaster(m_rasterKey);
	m_pContext->RSSetState(pState);
}

void CStateManagerDx11::SetFillMode(D3D11_FILL_MODE mode)
{
	if (m_rasterKey.fillMode == mode)
		return;

	m_rasterKey.fillMode = mode;

	ID3D11RasterizerState* pState = GetOrCreateRaster(m_rasterKey);
	m_pContext->RSSetState(pState);
}

void CStateManagerDx11::SetScissorEnable(BOOL enable)
{
	if (m_rasterKey.scissor == enable)
		return;

	m_rasterKey.scissor = enable;

	ID3D11RasterizerState* pState = GetOrCreateRaster(m_rasterKey);
	m_pContext->RSSetState(pState);
}

void CStateManagerDx11::SetViewport(float width, float height)
{
	D3D11_VIEWPORT vp = {};

	vp.Width = width;
	vp.Height = height;
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;

	m_pContext->RSSetViewports(1, &vp);
}

void CStateManagerDx11::Draw(UINT vertexCount, UINT startVertex)
{
	m_pContext->Draw(vertexCount, startVertex);
}

void CStateManagerDx11::DrawIndexed(UINT indexCount, UINT startIndex, INT baseVertex)
{
	m_pContext->DrawIndexed(indexCount, startIndex, baseVertex);
}
