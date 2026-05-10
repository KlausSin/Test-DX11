#include "StdAfx.h"
#include "GrpShadowTexture.h"
#include "EterBase/Stl.h"
#include "StateManager.h"

void CGraphicShadowTexture::Destroy()
{
	CGraphicTexture::Destroy();

	safe_release(m_pShadowRTV);
	safe_release(m_pShadowTex);
	safe_release(m_pDepthDSV);
	safe_release(m_pDepthTex);

	Initialize();
}

bool CGraphicShadowTexture::Create(int width, int height)
{
	Destroy();

	if (!ms_lpd3d11Device)
		return false;

	m_width = width;
	m_height = height;

	// Color render target
	D3D11_TEXTURE2D_DESC td = {};
	td.Width = width;
	td.Height = height;
	td.MipLevels = 1;
	td.ArraySize = 1;
	td.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	td.SampleDesc.Count = 1;
	td.Usage = D3D11_USAGE_DEFAULT;
	td.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&td, NULL, &m_pShadowTex)))
		return false;

	if (FAILED(ms_lpd3d11Device->CreateRenderTargetView(m_pShadowTex, NULL, &m_pShadowRTV)))
		return false;

	if (FAILED(ms_lpd3d11Device->CreateShaderResourceView(m_pShadowTex, NULL, &m_lpSRV)))
		return false;

	// Depth stencil
	D3D11_TEXTURE2D_DESC dd = {};
	dd.Width = width;
	dd.Height = height;
	dd.MipLevels = 1;
	dd.ArraySize = 1;
	dd.Format = DXGI_FORMAT_D16_UNORM;
	dd.SampleDesc.Count = 1;
	dd.Usage = D3D11_USAGE_DEFAULT;
	dd.BindFlags = D3D11_BIND_DEPTH_STENCIL;

	if (FAILED(ms_lpd3d11Device->CreateTexture2D(&dd, NULL, &m_pDepthTex)))
		return false;

	if (FAILED(ms_lpd3d11Device->CreateDepthStencilView(m_pDepthTex, NULL, &m_pDepthDSV)))
		return false;

	m_bEmpty = false;
	return true;
}

void CGraphicShadowTexture::Set(int stage) const
{
	STATEMANAGER.SetTexture(stage, m_lpSRV);
}

const D3DXMATRIX& CGraphicShadowTexture::GetLightVPMatrixReference() const
{
	return m_d3dLightVPMatrix;
}

void CGraphicShadowTexture::Begin()
{
	if (!ms_lpd3d11Context)
		return;

	D3DXMatrixMultiply(&m_d3dLightVPMatrix, &ms_matView, &ms_matProj);

	ms_lpd3d11Context->OMGetRenderTargets(1, &m_pOldRTV, &m_pOldDSV);
	m_uOldNumViewports = 1;
	ms_lpd3d11Context->RSGetViewports(&m_uOldNumViewports, &m_d3dOldViewport);

	ms_lpd3d11Context->OMSetRenderTargets(1, &m_pShadowRTV, m_pDepthDSV);

	D3D11_VIEWPORT vp = {};
	vp.MinDepth = 0.0f;
	vp.MaxDepth = 1.0f;
	vp.TopLeftX = 0;
	vp.TopLeftY = 0;
	vp.Width = (float)m_width;
	vp.Height = (float)m_height;
	ms_lpd3d11Context->RSSetViewports(1, &vp);

	float clearColor[4] = { 0, 0, 0, 0 };
	ms_lpd3d11Context->ClearRenderTargetView(m_pShadowRTV, clearColor);
	ms_lpd3d11Context->ClearDepthStencilView(m_pDepthDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

	STATEMANAGER.GetStateCache().Push();

	STATEMANAGER.GetRaster().SetCullMode(D3D11_CULL_NONE);
	STATEMANAGER.GetDepthStencil().SetDepthFunc(D3D11_COMPARISON_LESS_EQUAL);
	STATEMANAGER.GetBlend().SetBlendEnable(true);

	_mgr->GetCbMgr()->SetAlphaTestEnable(true);
	_mgr->GetCbMgr()->SetTextureFactor(0xbb000000);

	STATEMANAGER.SetTexture(0, NULL);
	STATEMANAGER.GetSampler().SetFilter(0, D3D11_FILTER_MIN_MAG_MIP_POINT);
	STATEMANAGER.GetSampler().SetAddressUV(0, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);

	STATEMANAGER.SetTexture(1, NULL);
	STATEMANAGER.GetSampler().SetFilter(1, D3D11_FILTER_MIN_MAG_MIP_POINT);
	STATEMANAGER.GetSampler().SetAddressUV(1, D3D11_TEXTURE_ADDRESS_CLAMP, D3D11_TEXTURE_ADDRESS_CLAMP);
}

void CGraphicShadowTexture::End()
{
	if (!ms_lpd3d11Context)
		return;

	ms_lpd3d11Context->OMSetRenderTargets(1, &m_pOldRTV, m_pOldDSV);
	ms_lpd3d11Context->RSSetViewports(1, &m_d3dOldViewport);

	safe_release(m_pOldRTV);
	safe_release(m_pOldDSV);

	STATEMANAGER.GetStateCache().Restore();
}

void CGraphicShadowTexture::Initialize()
{
	CGraphicTexture::Initialize();

	m_pShadowTex = NULL;
	m_pShadowRTV = NULL;
	m_pDepthTex = NULL;
	m_pDepthDSV = NULL;
	m_pOldRTV = NULL;
	m_pOldDSV = NULL;
	m_uOldNumViewports = 0;
	memset(&m_d3dOldViewport, 0, sizeof(m_d3dOldViewport));
}

CGraphicShadowTexture::CGraphicShadowTexture()
{
	Initialize();
}

CGraphicShadowTexture::~CGraphicShadowTexture()
{
	Destroy();
}
