#pragma once
#include "D3D11StateCommon.h"

static const UINT D3D11_SAMPLER_CACHE_SLOT_COUNT = 16;

struct SD3D11SamplerStateKey
{
	D3D11_FILTER filter;
	D3D11_TEXTURE_ADDRESS_MODE addressU;
	D3D11_TEXTURE_ADDRESS_MODE addressV;
	D3D11_TEXTURE_ADDRESS_MODE addressW;
	FLOAT mipLODBias;
	UINT maxAnisotropy;
	D3D11_COMPARISON_FUNC comparisonFunc;
	FLOAT borderColor[4];
	FLOAT minLOD;
	FLOAT maxLOD;

	SD3D11SamplerStateKey();
	bool operator==(const SD3D11SamplerStateKey& rhs) const;
};

class CD3D11SamplerStateCache
{
public:
	CD3D11SamplerStateCache();
	~CD3D11SamplerStateCache();

	void Initialize(ID3D11Device* device, ID3D11DeviceContext* context);
	void Destroy();
	void ResetDefault();
	void ForceDirty();
	void ForceDirty(UINT slot);
	bool IsDirty() const;
	bool IsDirty(UINT slot) const;
	bool CanRestore() const;
	bool CanRestore(UINT slot) const;
	void Push();
	void Push(UINT slot);
	bool Restore();
	bool Restore(UINT slot);
	void ClearStack();
	void ClearStack(UINT slot);

	void SetFilter(UINT slot, D3D11_FILTER filter);
	void SetAddressU(UINT slot, D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetAddressV(UINT slot, D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetAddressW(UINT slot, D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetAddressUV(UINT slot, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v);
	void SetAddressUVW(UINT slot, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w);
	void SetAddressAll(UINT slot, D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetMaxAnisotropy(UINT slot, UINT value);
	void SetComparisonFunc(UINT slot, D3D11_COMPARISON_FUNC func);
	void SetMipLODBias(UINT slot, FLOAT value);
	void SetMinLOD(UINT slot, FLOAT value);
	void SetMaxLOD(UINT slot, FLOAT value);
	void SetLOD(UINT slot, FLOAT minLOD, FLOAT maxLOD);
	void SetBorderColor(UINT slot, FLOAT r, FLOAT g, FLOAT b, FLOAT a);
	void SetKey(UINT slot, const SD3D11SamplerStateKey& key);

	void SetFilter(D3D11_FILTER filter);
	void SetAddressU(D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetAddressV(D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetAddressW(D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetAddressUV(D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v);
	void SetAddressUVW(D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w);
	void SetAddressAll(D3D11_TEXTURE_ADDRESS_MODE mode);
	void SetMaxAnisotropy(UINT value);
	void SetComparisonFunc(D3D11_COMPARISON_FUNC func);
	void SetMipLODBias(FLOAT value);
	void SetMinLOD(FLOAT value);
	void SetMaxLOD(FLOAT value);
	void SetLOD(FLOAT minLOD, FLOAT maxLOD);
	void SetBorderColor(FLOAT r, FLOAT g, FLOAT b, FLOAT a);
	void SetKey(const SD3D11SamplerStateKey& key);

	D3D11_FILTER GetFilter(UINT slot) const;
	D3D11_TEXTURE_ADDRESS_MODE GetAddressU(UINT slot) const;
	D3D11_TEXTURE_ADDRESS_MODE GetAddressV(UINT slot) const;
	D3D11_TEXTURE_ADDRESS_MODE GetAddressW(UINT slot) const;
	UINT GetMaxAnisotropy(UINT slot) const;
	D3D11_COMPARISON_FUNC GetComparisonFunc(UINT slot) const;
	FLOAT GetMipLODBias(UINT slot) const;
	FLOAT GetMinLOD(UINT slot) const;
	FLOAT GetMaxLOD(UINT slot) const;
	void GetBorderColor(UINT slot, FLOAT outColor[4]) const;
	const SD3D11SamplerStateKey& GetKey(UINT slot) const;
	ID3D11SamplerState* GetCurrentState(UINT slot);
	ID3D11SamplerState* GetOrCreate(const SD3D11SamplerStateKey& key);

	D3D11_FILTER GetFilter() const;
	D3D11_TEXTURE_ADDRESS_MODE GetAddressU() const;
	D3D11_TEXTURE_ADDRESS_MODE GetAddressV() const;
	D3D11_TEXTURE_ADDRESS_MODE GetAddressW() const;
	UINT GetMaxAnisotropy() const;
	D3D11_COMPARISON_FUNC GetComparisonFunc() const;
	FLOAT GetMipLODBias() const;
	FLOAT GetMinLOD() const;
	FLOAT GetMaxLOD() const;
	void GetBorderColor(FLOAT outColor[4]) const;
	const SD3D11SamplerStateKey& GetKey() const;
	ID3D11SamplerState* GetCurrentState();

	void ApplyPS(UINT slot);
	void ApplyVS(UINT slot);
	void ApplyGS(UINT slot);
	void ApplyHS(UINT slot);
	void ApplyDS(UINT slot);
	void ApplyCS(UINT slot);
	void ApplyAll(UINT slot);
	void Apply(UINT slot);
	void ApplyAllPS();
	void ApplyAllVS();
	void ApplyAllGS();
	void ApplyAllHS();
	void ApplyAllDS();
	void ApplyAllCS();
	void ApplyAllStages();

private:
	bool IsValidSlot(UINT slot) const;
	void MarkDirty(UINT slot);

	ID3D11Device* m_device;
	ID3D11DeviceContext* m_context;
	SD3D11SamplerStateKey m_key[D3D11_SAMPLER_CACHE_SLOT_COUNT];
	std::unordered_map<SD3D11SamplerStateKey, ID3D11SamplerState*, SD3D11RawHash> m_cache;
	std::vector<SD3D11SamplerStateKey> m_stack[D3D11_SAMPLER_CACHE_SLOT_COUNT];
	bool m_dirty[D3D11_SAMPLER_CACHE_SLOT_COUNT];
};
