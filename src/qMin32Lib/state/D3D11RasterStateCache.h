#pragma once
#include "D3D11StateCommon.h"

struct SD3D11RasterStateKey
{
	D3D11_FILL_MODE fillMode;
	D3D11_CULL_MODE cullMode;
	BOOL frontCounterClockwise;
	INT depthBias;
	FLOAT depthBiasClamp;
	FLOAT slopeScaledDepthBias;
	BOOL depthClipEnable;
	BOOL scissorEnable;
	BOOL multisampleEnable;
	BOOL antialiasedLineEnable;

	SD3D11RasterStateKey();
	bool operator==(const SD3D11RasterStateKey& rhs) const;
};

class CD3D11RasterStateCache
{
public:
	CD3D11RasterStateCache();
	~CD3D11RasterStateCache();

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
	void Destroy();
	void ResetDefault();
	void ForceDirty();
	bool IsDirty() const;
	bool CanRestore() const;
	void Push();
	bool Restore();
	void ClearStack();

	void SetFillMode(D3D11_FILL_MODE value);
	void SetCullMode(D3D11_CULL_MODE value);
	void SetFrontCounterClockwise(BOOL value);
	void SetDepthBias(INT value);
	void SetDepthBiasClamp(FLOAT value);
	void SetSlopeScaledDepthBias(FLOAT value);
	void SetDepthBiasAll(INT bias, FLOAT clampValue, FLOAT slopeValue);
	void SetDepthClipEnable(BOOL value);
	void SetScissorEnable(BOOL value);
	void SetMultisampleEnable(BOOL value);
	void SetAntialiasedLineEnable(BOOL value);
	void SetKey(const SD3D11RasterStateKey& key);

	D3D11_FILL_MODE GetFillMode() const;
	D3D11_CULL_MODE GetCullMode() const;
	BOOL GetFrontCounterClockwise() const;
	INT GetDepthBias() const;
	FLOAT GetDepthBiasClamp() const;
	FLOAT GetSlopeScaledDepthBias() const;
	BOOL GetDepthClipEnable() const;
	BOOL GetScissorEnable() const;
	BOOL GetMultisampleEnable() const;
	BOOL GetAntialiasedLineEnable() const;
	const SD3D11RasterStateKey& GetKey() const;
	ID3D11RasterizerState* GetCurrentState();
	ID3D11RasterizerState* GetOrCreate(const SD3D11RasterStateKey& key);
	void Apply();

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	SD3D11RasterStateKey m_key;
	std::unordered_map<SD3D11RasterStateKey, ID3D11RasterizerState*, SD3D11RawHash> m_cache;
	std::vector<SD3D11RasterStateKey> m_stack;
	bool m_dirty;
};
