#pragma once
#include "D3D11StateCommon.h"

struct SD3D11DepthStencilStateKey
{
	BOOL depthEnable;
	D3D11_DEPTH_WRITE_MASK depthWriteMask;
	D3D11_COMPARISON_FUNC depthFunc;
	BOOL stencilEnable;
	UINT8 stencilReadMask;
	UINT8 stencilWriteMask;
	D3D11_DEPTH_STENCILOP_DESC frontFace;
	D3D11_DEPTH_STENCILOP_DESC backFace;

	SD3D11DepthStencilStateKey();
	bool operator==(const SD3D11DepthStencilStateKey& rhs) const;
};

struct SD3D11DepthStencilRuntimeState
{
	SD3D11DepthStencilStateKey key;
	UINT stencilRef;
};

class CD3D11DepthStencilStateCache
{
public:
	CD3D11DepthStencilStateCache();
	~CD3D11DepthStencilStateCache();

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
	void Destroy();
	void ResetDefault();
	void ForceDirty();
	bool IsDirty() const;
	bool CanRestore() const;
	void Push();
	bool Restore();
	void ClearStack();

	void SetDepthEnable(BOOL value);
	void SetDepthWriteEnable(BOOL value);
	void SetDepthWriteMask(D3D11_DEPTH_WRITE_MASK value);
	void SetDepthFunc(D3D11_COMPARISON_FUNC value);
	void SetStencilEnable(BOOL value);
	void SetStencilReadMask(UINT8 value);
	void SetStencilWriteMask(UINT8 value);
	void SetStencilMasks(UINT8 readMask, UINT8 writeMask);
	void SetStencilRef(UINT value);
	void SetFrontFace(const D3D11_DEPTH_STENCILOP_DESC& value);
	void SetBackFace(const D3D11_DEPTH_STENCILOP_DESC& value);
	void SetStencilOps(D3D11_STENCIL_OP failOp, D3D11_STENCIL_OP depthFailOp, D3D11_STENCIL_OP passOp, D3D11_COMPARISON_FUNC func);
	void SetFrontStencilOps(D3D11_STENCIL_OP failOp, D3D11_STENCIL_OP depthFailOp, D3D11_STENCIL_OP passOp, D3D11_COMPARISON_FUNC func);
	void SetBackStencilOps(D3D11_STENCIL_OP failOp, D3D11_STENCIL_OP depthFailOp, D3D11_STENCIL_OP passOp, D3D11_COMPARISON_FUNC func);
	void SetKey(const SD3D11DepthStencilStateKey& key);

	BOOL GetDepthEnable() const;
	BOOL GetDepthWriteEnable() const;
	D3D11_DEPTH_WRITE_MASK GetDepthWriteMask() const;
	D3D11_COMPARISON_FUNC GetDepthFunc() const;
	BOOL GetStencilEnable() const;
	UINT8 GetStencilReadMask() const;
	UINT8 GetStencilWriteMask() const;
	UINT GetStencilRef() const;
	const D3D11_DEPTH_STENCILOP_DESC& GetFrontFace() const;
	const D3D11_DEPTH_STENCILOP_DESC& GetBackFace() const;
	const SD3D11DepthStencilStateKey& GetKey() const;
	ID3D11DepthStencilState* GetCurrentState();
	ID3D11DepthStencilState* GetOrCreate(const SD3D11DepthStencilStateKey& key);
	void Apply();

private:
	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	SD3D11DepthStencilStateKey m_key;
	std::unordered_map<SD3D11DepthStencilStateKey, ID3D11DepthStencilState*, SD3D11RawHash> m_cache;
	std::vector<SD3D11DepthStencilRuntimeState> m_stack;
	UINT m_stencilRef;
	bool m_dirty;
};
