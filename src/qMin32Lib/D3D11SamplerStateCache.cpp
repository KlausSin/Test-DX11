#include "D3D11SamplerStateCache.h"

SD3D11SamplerStateKey::SD3D11SamplerStateKey()
{
	std::memset(this, 0, sizeof(*this));
	filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	addressU = D3D11_TEXTURE_ADDRESS_WRAP;
	addressV = D3D11_TEXTURE_ADDRESS_WRAP;
	addressW = D3D11_TEXTURE_ADDRESS_WRAP;
	mipLODBias = 0.0f;
	maxAnisotropy = 1;
	comparisonFunc = D3D11_COMPARISON_NEVER;
	borderColor[0] = 0.0f;
	borderColor[1] = 0.0f;
	borderColor[2] = 0.0f;
	borderColor[3] = 0.0f;
	minLOD = 0.0f;
	maxLOD = D3D11_FLOAT32_MAX;
}

bool SD3D11SamplerStateKey::operator==(const SD3D11SamplerStateKey& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(*this)) == 0;
}

CD3D11SamplerStateCache::CD3D11SamplerStateCache() : m_device(nullptr), m_context(nullptr)
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		m_dirty[i] = true;
}

CD3D11SamplerStateCache::~CD3D11SamplerStateCache()
{
	Destroy();
}

bool CD3D11SamplerStateCache::IsValidSlot(UINT slot) const
{
	return slot < D3D11_SAMPLER_CACHE_SLOT_COUNT;
}

void CD3D11SamplerStateCache::MarkDirty(UINT slot)
{
	if (IsValidSlot(slot))
		m_dirty[slot] = true;
}

void CD3D11SamplerStateCache::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_device = device;
	m_context = context;
	ForceDirty();
}

void CD3D11SamplerStateCache::Destroy()
{
	for (auto& it : m_cache)
		SAFE_RELEASE_D3D11(it.second);
	m_cache.clear();
	ClearStack();
	m_device = nullptr;
	m_context = nullptr;
	ForceDirty();
}

void CD3D11SamplerStateCache::ResetDefault()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
	{
		m_key[i] = SD3D11SamplerStateKey();
		m_dirty[i] = true;
	}
}

void CD3D11SamplerStateCache::ForceDirty()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		m_dirty[i] = true;
}

void CD3D11SamplerStateCache::ForceDirty(UINT slot)
{
	MarkDirty(slot);
}

bool CD3D11SamplerStateCache::IsDirty() const
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		if (m_dirty[i])
			return true;
	return false;
}

bool CD3D11SamplerStateCache::IsDirty(UINT slot) const
{
	return IsValidSlot(slot) && m_dirty[slot];
}

bool CD3D11SamplerStateCache::CanRestore() const
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		if (m_stack[i].empty())
			return false;
	return true;
}

bool CD3D11SamplerStateCache::CanRestore(UINT slot) const
{
	return IsValidSlot(slot) && !m_stack[slot].empty();
}

void CD3D11SamplerStateCache::Push()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		m_stack[i].push_back(m_key[i]);
}

void CD3D11SamplerStateCache::Push(UINT slot)
{
	if (IsValidSlot(slot))
		m_stack[slot].push_back(m_key[slot]);
}

bool CD3D11SamplerStateCache::Restore()
{
	if (!CanRestore())
		return false;
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
	{
		SetKey(i, m_stack[i].back());
		m_stack[i].pop_back();
	}
	return true;
}

bool CD3D11SamplerStateCache::Restore(UINT slot)
{
	if (!CanRestore(slot))
		return false;
	SetKey(slot, m_stack[slot].back());
	m_stack[slot].pop_back();
	return true;
}

void CD3D11SamplerStateCache::ClearStack()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		m_stack[i].clear();
}

void CD3D11SamplerStateCache::ClearStack(UINT slot)
{
	if (IsValidSlot(slot))
		m_stack[slot].clear();
}

void CD3D11SamplerStateCache::SetFilter(UINT slot, D3D11_FILTER v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].filter != v)
	{
		m_key[slot].filter = v;
		if (v == D3D11_FILTER_ANISOTROPIC && m_key[slot].maxAnisotropy < 2)
			m_key[slot].maxAnisotropy = 8;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetAddressU(UINT slot, D3D11_TEXTURE_ADDRESS_MODE v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].addressU != v)
	{
		m_key[slot].addressU = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetAddressV(UINT slot, D3D11_TEXTURE_ADDRESS_MODE v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].addressV != v)
	{
		m_key[slot].addressV = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetAddressW(UINT slot, D3D11_TEXTURE_ADDRESS_MODE v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].addressW != v)
	{
		m_key[slot].addressW = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetAddressUV(UINT slot, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v)
{
	SetAddressU(slot, u);
	SetAddressV(slot, v);
}

void CD3D11SamplerStateCache::SetAddressUVW(UINT slot, D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w)
{
	SetAddressU(slot, u);
	SetAddressV(slot, v);
	SetAddressW(slot, w);
}

void CD3D11SamplerStateCache::SetAddressAll(UINT slot, D3D11_TEXTURE_ADDRESS_MODE v)
{
	SetAddressUVW(slot, v, v, v);
}

void CD3D11SamplerStateCache::SetMaxAnisotropy(UINT slot, UINT v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].maxAnisotropy != v)
	{
		m_key[slot].maxAnisotropy = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetComparisonFunc(UINT slot, D3D11_COMPARISON_FUNC v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].comparisonFunc != v)
	{
		m_key[slot].comparisonFunc = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetMipLODBias(UINT slot, FLOAT v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].mipLODBias != v)
	{
		m_key[slot].mipLODBias = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetMinLOD(UINT slot, FLOAT v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].minLOD != v)
	{
		m_key[slot].minLOD = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetMaxLOD(UINT slot, FLOAT v)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].maxLOD != v)
	{
		m_key[slot].maxLOD = v;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetLOD(UINT slot, FLOAT a, FLOAT b)
{
	SetMinLOD(slot, a);
	SetMaxLOD(slot, b);
}

void CD3D11SamplerStateCache::SetBorderColor(UINT slot, FLOAT r, FLOAT g, FLOAT b, FLOAT a)
{
	if (!IsValidSlot(slot)) return;
	if (m_key[slot].borderColor[0] != r || m_key[slot].borderColor[1] != g || m_key[slot].borderColor[2] != b || m_key[slot].borderColor[3] != a)
	{
		m_key[slot].borderColor[0] = r;
		m_key[slot].borderColor[1] = g;
		m_key[slot].borderColor[2] = b;
		m_key[slot].borderColor[3] = a;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetKey(UINT slot, const SD3D11SamplerStateKey& key)
{
	if (!IsValidSlot(slot)) return;
	if (!(m_key[slot] == key))
	{
		m_key[slot] = key;
		m_dirty[slot] = true;
	}
}

void CD3D11SamplerStateCache::SetFilter(D3D11_FILTER v) { SetFilter(0, v); }
void CD3D11SamplerStateCache::SetAddressU(D3D11_TEXTURE_ADDRESS_MODE v) { SetAddressU(0, v); }
void CD3D11SamplerStateCache::SetAddressV(D3D11_TEXTURE_ADDRESS_MODE v) { SetAddressV(0, v); }
void CD3D11SamplerStateCache::SetAddressW(D3D11_TEXTURE_ADDRESS_MODE v) { SetAddressW(0, v); }
void CD3D11SamplerStateCache::SetAddressUV(D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v) { SetAddressUV(0, u, v); }
void CD3D11SamplerStateCache::SetAddressUVW(D3D11_TEXTURE_ADDRESS_MODE u, D3D11_TEXTURE_ADDRESS_MODE v, D3D11_TEXTURE_ADDRESS_MODE w) { SetAddressUVW(0, u, v, w); }
void CD3D11SamplerStateCache::SetAddressAll(D3D11_TEXTURE_ADDRESS_MODE v) { SetAddressAll(0, v); }
void CD3D11SamplerStateCache::SetMaxAnisotropy(UINT v) { SetMaxAnisotropy(0, v); }
void CD3D11SamplerStateCache::SetComparisonFunc(D3D11_COMPARISON_FUNC v) { SetComparisonFunc(0, v); }
void CD3D11SamplerStateCache::SetMipLODBias(FLOAT v) { SetMipLODBias(0, v); }
void CD3D11SamplerStateCache::SetMinLOD(FLOAT v) { SetMinLOD(0, v); }
void CD3D11SamplerStateCache::SetMaxLOD(FLOAT v) { SetMaxLOD(0, v); }
void CD3D11SamplerStateCache::SetLOD(FLOAT a, FLOAT b) { SetLOD(0, a, b); }
void CD3D11SamplerStateCache::SetBorderColor(FLOAT r, FLOAT g, FLOAT b, FLOAT a) { SetBorderColor(0, r, g, b, a); }
void CD3D11SamplerStateCache::SetKey(const SD3D11SamplerStateKey& key) { SetKey(0, key); }

D3D11_FILTER CD3D11SamplerStateCache::GetFilter(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].filter : D3D11_FILTER_MIN_MAG_MIP_LINEAR; }
D3D11_TEXTURE_ADDRESS_MODE CD3D11SamplerStateCache::GetAddressU(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].addressU : D3D11_TEXTURE_ADDRESS_WRAP; }
D3D11_TEXTURE_ADDRESS_MODE CD3D11SamplerStateCache::GetAddressV(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].addressV : D3D11_TEXTURE_ADDRESS_WRAP; }
D3D11_TEXTURE_ADDRESS_MODE CD3D11SamplerStateCache::GetAddressW(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].addressW : D3D11_TEXTURE_ADDRESS_WRAP; }
UINT CD3D11SamplerStateCache::GetMaxAnisotropy(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].maxAnisotropy : 1; }
D3D11_COMPARISON_FUNC CD3D11SamplerStateCache::GetComparisonFunc(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].comparisonFunc : D3D11_COMPARISON_NEVER; }
FLOAT CD3D11SamplerStateCache::GetMipLODBias(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].mipLODBias : 0.0f; }
FLOAT CD3D11SamplerStateCache::GetMinLOD(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].minLOD : 0.0f; }
FLOAT CD3D11SamplerStateCache::GetMaxLOD(UINT slot) const { return IsValidSlot(slot) ? m_key[slot].maxLOD : D3D11_FLOAT32_MAX; }

void CD3D11SamplerStateCache::GetBorderColor(UINT slot, FLOAT outColor[4]) const
{
	if (!outColor) return;
	if (!IsValidSlot(slot))
	{
		outColor[0] = outColor[1] = outColor[2] = outColor[3] = 0.0f;
		return;
	}
	outColor[0] = m_key[slot].borderColor[0];
	outColor[1] = m_key[slot].borderColor[1];
	outColor[2] = m_key[slot].borderColor[2];
	outColor[3] = m_key[slot].borderColor[3];
}

const SD3D11SamplerStateKey& CD3D11SamplerStateCache::GetKey(UINT slot) const
{
	return m_key[IsValidSlot(slot) ? slot : 0];
}

ID3D11SamplerState* CD3D11SamplerStateCache::GetCurrentState(UINT slot)
{
	if (!IsValidSlot(slot)) return nullptr;
	return GetOrCreate(m_key[slot]);
}

D3D11_FILTER CD3D11SamplerStateCache::GetFilter() const { return GetFilter(0); }
D3D11_TEXTURE_ADDRESS_MODE CD3D11SamplerStateCache::GetAddressU() const { return GetAddressU(0); }
D3D11_TEXTURE_ADDRESS_MODE CD3D11SamplerStateCache::GetAddressV() const { return GetAddressV(0); }
D3D11_TEXTURE_ADDRESS_MODE CD3D11SamplerStateCache::GetAddressW() const { return GetAddressW(0); }
UINT CD3D11SamplerStateCache::GetMaxAnisotropy() const { return GetMaxAnisotropy(0); }
D3D11_COMPARISON_FUNC CD3D11SamplerStateCache::GetComparisonFunc() const { return GetComparisonFunc(0); }
FLOAT CD3D11SamplerStateCache::GetMipLODBias() const { return GetMipLODBias(0); }
FLOAT CD3D11SamplerStateCache::GetMinLOD() const { return GetMinLOD(0); }
FLOAT CD3D11SamplerStateCache::GetMaxLOD() const { return GetMaxLOD(0); }
void CD3D11SamplerStateCache::GetBorderColor(FLOAT outColor[4]) const { GetBorderColor(0, outColor); }
const SD3D11SamplerStateKey& CD3D11SamplerStateCache::GetKey() const { return GetKey(0); }
ID3D11SamplerState* CD3D11SamplerStateCache::GetCurrentState() { return GetCurrentState(0); }

ID3D11SamplerState* CD3D11SamplerStateCache::GetOrCreate(const SD3D11SamplerStateKey& key)
{
	auto it = m_cache.find(key);
	if (it != m_cache.end())
		return it->second;

	D3D11_SAMPLER_DESC d = {};
	d.Filter = key.filter;
	d.AddressU = key.addressU;
	d.AddressV = key.addressV;
	d.AddressW = key.addressW;
	d.MipLODBias = key.mipLODBias;
	d.MaxAnisotropy = key.maxAnisotropy;
	d.ComparisonFunc = key.comparisonFunc;
	d.BorderColor[0] = key.borderColor[0];
	d.BorderColor[1] = key.borderColor[1];
	d.BorderColor[2] = key.borderColor[2];
	d.BorderColor[3] = key.borderColor[3];
	d.MinLOD = key.minLOD;
	d.MaxLOD = key.maxLOD;

	ID3D11SamplerState* state = nullptr;
	if (!m_device || FAILED(m_device->CreateSamplerState(&d, &state)))
		return nullptr;

	m_cache[key] = state;
	return state;
}

void CD3D11SamplerStateCache::ApplyPS(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->PSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::ApplyVS(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->VSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::ApplyGS(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->GSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::ApplyHS(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->HSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::ApplyDS(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->DSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::ApplyCS(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->CSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::ApplyAll(UINT slot)
{
	if (!m_context || !IsValidSlot(slot) || !m_dirty[slot]) return;
	ID3D11SamplerState* s = GetCurrentState(slot);
	m_context->PSSetSamplers(slot, 1, &s);
	m_context->VSSetSamplers(slot, 1, &s);
	m_context->GSSetSamplers(slot, 1, &s);
	m_context->HSSetSamplers(slot, 1, &s);
	m_context->DSSetSamplers(slot, 1, &s);
	m_context->CSSetSamplers(slot, 1, &s);
	m_dirty[slot] = false;
}

void CD3D11SamplerStateCache::Apply(UINT slot)
{
	ApplyPS(slot);
}

void CD3D11SamplerStateCache::ApplyAllPS()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyPS(i);
}

void CD3D11SamplerStateCache::ApplyAllVS()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyVS(i);
}

void CD3D11SamplerStateCache::ApplyAllGS()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyGS(i);
}

void CD3D11SamplerStateCache::ApplyAllHS()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyHS(i);
}

void CD3D11SamplerStateCache::ApplyAllDS()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyDS(i);
}

void CD3D11SamplerStateCache::ApplyAllCS()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyCS(i);
}

void CD3D11SamplerStateCache::ApplyAllStages()
{
	for (UINT i = 0; i < D3D11_SAMPLER_CACHE_SLOT_COUNT; ++i)
		ApplyAll(i);
}
