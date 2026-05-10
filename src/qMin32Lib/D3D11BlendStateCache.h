#pragma once
#include "D3D11StateCommon.h"

struct SD3D11BlendStateKey
{
	BOOL alphaToCoverageEnable;
	BOOL independentBlendEnable;
	D3D11_RENDER_TARGET_BLEND_DESC renderTarget[8];

	SD3D11BlendStateKey();
	bool operator==(const SD3D11BlendStateKey& rhs) const;
};

struct SD3D11BlendRuntimeState
{
	SD3D11BlendStateKey key;
	FLOAT blendFactor[4];
	UINT sampleMask;
};

class CD3D11BlendStateCache
{
public:
	CD3D11BlendStateCache();
	~CD3D11BlendStateCache();

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
	void Destroy();
	void ResetDefault();
	void ForceDirty();
	bool IsDirty() const;
	bool CanRestore() const;
	void Push();
	bool Restore();
	void ClearStack();

	void SetAlphaToCoverageEnable(BOOL value);
	void SetIndependentBlendEnable(BOOL value);
	void SetBlendEnable(BOOL value);
	void SetSrcBlend(D3D11_BLEND value);
	void SetDestBlend(D3D11_BLEND value);
	void SetBlendOp( D3D11_BLEND_OP value);
	void SetSrcBlendAlpha(D3D11_BLEND value);
	void SetDestBlendAlpha(D3D11_BLEND value);
	void SetBlendOpAlpha(D3D11_BLEND_OP value);
	void SetColorWriteMask(UINT8 value);
	void SetRenderTarget(const D3D11_RENDER_TARGET_BLEND_DESC& value);
	void SetBlendFactor(FLOAT r, FLOAT g, FLOAT b, FLOAT a);
	void SetBlendFactorR(FLOAT value);
	void SetBlendFactorG(FLOAT value);
	void SetBlendFactorB(FLOAT value);
	void SetBlendFactorA(FLOAT value);
	void SetSampleMask(UINT value);
	void SetKey(const SD3D11BlendStateKey& key);

	BOOL GetAlphaToCoverageEnable() const;
	BOOL GetIndependentBlendEnable() const;
	BOOL GetBlendEnable() const;
	D3D11_BLEND GetSrcBlend() const;
	D3D11_BLEND GetDestBlend() const;
	D3D11_BLEND_OP GetBlendOp() const;
	D3D11_BLEND GetSrcBlendAlpha() const;
	D3D11_BLEND GetDestBlendAlpha() const;
	D3D11_BLEND_OP GetBlendOpAlpha() const;
	UINT8 GetColorWriteMask() const;
	const D3D11_RENDER_TARGET_BLEND_DESC& GetRenderTarget() const;
	void GetBlendFactor(FLOAT outFactor[4]) const;
	UINT GetSampleMask() const;
	const SD3D11BlendStateKey& GetKey() const;
	ID3D11BlendState* GetCurrentState();
	ID3D11BlendState* GetOrCreate(const SD3D11BlendStateKey& key);
	void Apply();

private:
	bool IsValidRT(UINT index) const;

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	SD3D11BlendStateKey m_key;
	std::unordered_map<SD3D11BlendStateKey, ID3D11BlendState*, SD3D11RawHash> m_cache;
	std::vector<SD3D11BlendRuntimeState> m_stack;
	FLOAT m_blendFactor[4];
	UINT m_sampleMask;
	bool m_dirty;
};
