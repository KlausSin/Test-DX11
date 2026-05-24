#pragma once

#include <d3d11.h>
#include <d3dcompiler.h>
#include "GrpBase.h"
#include "qMin32Lib/state/D3D11StateCacheSet.h"
#include "qMin32Lib/Core.h"

class CD3D11Renderer
{
public:
	CD3D11Renderer();
	~CD3D11Renderer();

	bool Initialize(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	void Destroy();

	CBManagerPtr GetCbMgr();

	CD3D11StateCacheSet& GetStateCache() { return m_StateCache; }
	const CD3D11StateCacheSet& GetStateCache() const { return m_StateCache; }

	void PushStateCache() { m_StateCache.Push(); }
	bool RestoreStateCache() { bool ok = m_StateCache.Restore(); m_StateCache.ForceDirty(); return ok; }
	void ClearStateCacheStacks() { m_StateCache.ClearStacks(); }
	void ResetStateCacheDefault() { m_StateCache.ResetDefault(); m_StateCache.ForceDirty(); }
	void ForceStateCacheDirty() { m_StateCache.ForceDirty(); }

	void SetTexture(DWORD dwStage, ID3D11ShaderResourceView* pSRV);
	void FlushAllState();

private:
	ComPtr<ID3D11Device> m_pDevice = nullptr;
	ComPtr<ID3D11DeviceContext> m_pContext = nullptr;
	CD3D11StateCacheSet m_StateCache;
	bool m_bInitialized = false;
};
