#include "StdAfx.h"
#include "GrpD3D11Renderer.h"
#include "EterBase/Stl.h"
#include "EterBase/Debug.h"
#include "qMin32Lib/ConstantBuffer.h"
#include "qMin32Lib/DxManager.h"
#include "qMin32Lib/ConstantBufferManager.h"
#include "GrpBase.h"

CD3D11Renderer::CD3D11Renderer()
{
}

CD3D11Renderer::~CD3D11Renderer()
{
	Destroy();
}

bool CD3D11Renderer::Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	m_pDevice = pDevice;
	m_pContext = pContext;

	m_StateCache.Initialize(pDevice, pContext);
	m_StateCache.ResetDefault();

	_mgr->SetShader(VF_PDT, BLEND_MODULATE);

	m_StateCache.Blend.SetBlendEnable(FALSE);
	m_StateCache.Blend.SetSrcBlend(D3D11_BLEND_SRC_ALPHA);
	m_StateCache.Blend.SetDestBlend(D3D11_BLEND_INV_SRC_ALPHA);
	m_StateCache.Blend.SetBlendOp(D3D11_BLEND_OP_ADD);
	m_StateCache.Blend.SetSrcBlendAlpha(D3D11_BLEND_ONE);
	m_StateCache.Blend.SetDestBlendAlpha(D3D11_BLEND_INV_SRC_ALPHA);
	m_StateCache.Blend.SetBlendOpAlpha(D3D11_BLEND_OP_ADD);
	m_StateCache.Blend.SetColorWriteMask(D3D11_COLOR_WRITE_ENABLE_ALL);

	m_StateCache.DepthStencil.SetDepthEnable(TRUE);
	m_StateCache.DepthStencil.SetDepthWriteEnable(TRUE);
	m_StateCache.DepthStencil.SetDepthFunc(D3D11_COMPARISON_LESS_EQUAL);
	m_StateCache.DepthStencil.SetStencilEnable(FALSE);

	m_StateCache.Raster.SetFillMode(D3D11_FILL_SOLID);
	m_StateCache.Raster.SetCullMode(D3D11_CULL_NONE);
	m_StateCache.Raster.SetScissorEnable(FALSE);
	m_StateCache.Raster.SetDepthClipEnable(TRUE);

	m_StateCache.Sampler.SetFilter(D3D11_FILTER_MIN_MAG_MIP_LINEAR);
	m_StateCache.Sampler.SetAddressAll(D3D11_TEXTURE_ADDRESS_WRAP);
	m_StateCache.Sampler.SetMaxAnisotropy(1);
	m_StateCache.Sampler.SetComparisonFunc(D3D11_COMPARISON_NEVER);
	m_StateCache.Sampler.SetLOD(0.0f, D3D11_FLOAT32_MAX);

	_mgr->GetCbMgr()->m_cbMaterial.textureFactor[0] = 1.0f;
	_mgr->GetCbMgr()->m_cbMaterial.textureFactor[1] = 1.0f;
	_mgr->GetCbMgr()->m_cbMaterial.textureFactor[2] = 1.0f;
	_mgr->GetCbMgr()->m_cbMaterial.textureFactor[3] = 1.0f;
	_mgr->GetCbMgr()->m_cbMaterial.useTexture0 = 1;
	_mgr->GetCbMgr()->m_cbMaterial.useTexture1 = 0;

	_mgr->GetCbMgr()->m_cbLighting.lightAmbient[0] = 0.0f;
	_mgr->GetCbMgr()->m_cbLighting.lightAmbient[1] = 0.0f;
	_mgr->GetCbMgr()->m_cbLighting.lightAmbient[2] = 0.0f;
	_mgr->GetCbMgr()->m_cbLighting.lightAmbient[3] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matDiffuse[0] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matDiffuse[1] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matDiffuse[2] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matDiffuse[3] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matAmbient[0] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matAmbient[1] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matAmbient[2] = 1.0f;
	_mgr->GetCbMgr()->m_cbLighting.matAmbient[3] = 1.0f;

	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.frame.matWorld, XMMatrixIdentity());
	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.frame.matView, XMMatrixIdentity());
	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.frame.matProj, XMMatrixIdentity());

	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.texTransform.tex0, XMMatrixIdentity());
	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.texTransform.tex1, XMMatrixIdentity());
	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.texTransform.tex2, XMMatrixIdentity());
	XMStoreFloat4x4(&_mgr->GetCbMgr()->m_cbMatrix.texTransform.tex3, XMMatrixIdentity());

	_mgr->GetCbMgr()->m_bMatrixDirty = true;
	_mgr->GetCbMgr()->m_bMaterialDirty = true;
	_mgr->GetCbMgr()->m_bLightingDirty = true;
	_mgr->GetCbMgr()->m_bFogDirty = true;

	m_StateCache.ForceDirty();
	m_bInitialized = true;

	FlushAllState();
	_mgr->GetCbMgr()->SetAllBuffers();

	Tracenf("D3D11Renderer: Initialization complete");
	return true;
}

void CD3D11Renderer::Destroy()
{
	m_StateCache.Destroy();
	m_pContext = nullptr;
	m_pDevice = nullptr;
	m_bInitialized = false;
}

CBManagerPtr CD3D11Renderer::GetCbMgr()
{
	return _mgr->GetCbMgr();
}

void CD3D11Renderer::SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV)
{
	if (dwStage == 0)
		_mgr->GetCbMgr()->m_cbMaterial.useTexture0 = pSRV ? 1 : 0;
	else if (dwStage == 1)
		_mgr->GetCbMgr()->m_cbMaterial.useTexture1 = pSRV ? 1 : 0;

	_mgr->GetCbMgr()->m_bMaterialDirty = true;

	if (!m_pContext)
		return;

	ID3D11ShaderResourceView* srv = pSRV;
	m_pContext->PSSetShaderResources(dwStage, 1, &srv);
}

void CD3D11Renderer::FlushAllState()
{
	_mgr->GetCbMgr()->FlushAllState();
	m_StateCache.Apply();
}
