#include "D3D11RasterStateCache.h"
#include <EterBase/Debug.h>
#include <assert.h>

SD3D11RasterStateKey::SD3D11RasterStateKey()
{
	std::memset(this, 0, sizeof(*this));
	fillMode = D3D11_FILL_SOLID;
	cullMode = D3D11_CULL_NONE;
	depthClipEnable = TRUE;
}

bool SD3D11RasterStateKey::operator==(const SD3D11RasterStateKey& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(*this)) == 0;
}

CD3D11RasterStateCache::CD3D11RasterStateCache() : m_device(nullptr), m_context(nullptr), m_dirty(true)
{
}

CD3D11RasterStateCache::~CD3D11RasterStateCache()
{
	Destroy();
}

void CD3D11RasterStateCache::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_device = device;
	m_context = context;
	m_dirty = true;
}

void CD3D11RasterStateCache::Destroy()
{
	for (auto& it : m_cache)
		SAFE_RELEASE_D3D11(it.second);
	m_cache.clear();
	m_stack.clear();
	m_device = nullptr;
	m_context = nullptr;
	m_dirty = true;
}

void CD3D11RasterStateCache::ResetDefault()
{
	m_key = SD3D11RasterStateKey();
	m_dirty = true;
}

void CD3D11RasterStateCache::ForceDirty()
{
	m_dirty = true;
}

bool CD3D11RasterStateCache::IsDirty() const
{
	return m_dirty;
}

bool CD3D11RasterStateCache::CanRestore() const
{
	return !m_stack.empty();
}

void CD3D11RasterStateCache::Push()
{
	m_stack.push_back(m_key);
}

bool CD3D11RasterStateCache::Restore()
{
#ifdef _DEBUG
	if (m_stack.empty())
	{
		Tracef("CD3D11RasterStateCache::Restore - state was not pushed\n");
		assert(!"CD3D11RasterStateCache::Restore - state was not pushed");
		return false;
	}
#else
	if (m_stack.empty())
		return false;
#endif
	SetKey(m_stack.back());
	m_stack.pop_back();
	return true;
}

void CD3D11RasterStateCache::ClearStack()
{
	m_stack.clear();
}

void CD3D11RasterStateCache::SetFillMode(D3D11_FILL_MODE v)
{
	if (m_key.fillMode != v)
	{
		m_key.fillMode = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetCullMode(D3D11_CULL_MODE v)
{
	if (m_key.cullMode != v)
	{
		m_key.cullMode = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetFrontCounterClockwise(BOOL v)
{
	if (m_key.frontCounterClockwise != v)
	{
		m_key.frontCounterClockwise = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetDepthBias(INT v)
{
	if (m_key.depthBias != v)
	{
		m_key.depthBias = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetDepthBiasClamp(FLOAT v)
{
	if (m_key.depthBiasClamp != v)
	{
		m_key.depthBiasClamp = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetSlopeScaledDepthBias(FLOAT v)
{
	if (m_key.slopeScaledDepthBias != v)
	{
		m_key.slopeScaledDepthBias = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetDepthBiasAll(INT a, FLOAT b, FLOAT c)
{
	SetDepthBias(a);
	SetDepthBiasClamp(b);
	SetSlopeScaledDepthBias(c);
}

void CD3D11RasterStateCache::SetDepthClipEnable(BOOL v)
{
	if (m_key.depthClipEnable != v)
	{
		m_key.depthClipEnable = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetScissorEnable(BOOL v)
{
	if (m_key.scissorEnable != v)
	{
		m_key.scissorEnable = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetMultisampleEnable(BOOL v)
{
	if (m_key.multisampleEnable != v)
	{
		m_key.multisampleEnable = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetAntialiasedLineEnable(BOOL v)
{ 
	if (m_key.antialiasedLineEnable != v) 
	{
		m_key.antialiasedLineEnable = v;
		m_dirty = true;
	}
}

void CD3D11RasterStateCache::SetKey(const SD3D11RasterStateKey& key)
{
	if (!(m_key == key))
	{
		m_key = key;
		m_dirty = true;
	}
}

D3D11_FILL_MODE CD3D11RasterStateCache::GetFillMode() const
{
	return m_key.fillMode;
}

D3D11_CULL_MODE CD3D11RasterStateCache::GetCullMode() const 
{ 
	return m_key.cullMode; 
}

BOOL CD3D11RasterStateCache::GetFrontCounterClockwise() const
{ 
	return m_key.frontCounterClockwise;
}

INT CD3D11RasterStateCache::GetDepthBias() const
{ 
	return m_key.depthBias;
}

FLOAT CD3D11RasterStateCache::GetDepthBiasClamp() const
{ 
	return m_key.depthBiasClamp;
}

FLOAT CD3D11RasterStateCache::GetSlopeScaledDepthBias() const
{
	return m_key.slopeScaledDepthBias;
}

BOOL CD3D11RasterStateCache::GetDepthClipEnable() const 
{ 
	return m_key.depthClipEnable;
}

BOOL CD3D11RasterStateCache::GetScissorEnable() const 
{
	return m_key.scissorEnable;
}

BOOL CD3D11RasterStateCache::GetMultisampleEnable() const 
{
	return m_key.multisampleEnable;
}

BOOL CD3D11RasterStateCache::GetAntialiasedLineEnable() const 
{ 
	return m_key.antialiasedLineEnable;
}

const SD3D11RasterStateKey& CD3D11RasterStateCache::GetKey() const 
{ 
	return m_key;
}

ID3D11RasterizerState* CD3D11RasterStateCache::GetCurrentState()
{ 
	return GetOrCreate(m_key);
}

ID3D11RasterizerState* CD3D11RasterStateCache::GetOrCreate(const SD3D11RasterStateKey& key)
{
	auto it = m_cache.find(key);
	if (it != m_cache.end()) 
		return it->second;
	D3D11_RASTERIZER_DESC d = {};
	d.FillMode = key.fillMode;
	d.CullMode = key.cullMode; 
	d.FrontCounterClockwise = key.frontCounterClockwise;
	d.DepthBias = key.depthBias;
	d.DepthBiasClamp = key.depthBiasClamp; 
	d.SlopeScaledDepthBias = key.slopeScaledDepthBias;
	d.DepthClipEnable = key.depthClipEnable;
	d.ScissorEnable = key.scissorEnable;
	d.MultisampleEnable = key.multisampleEnable;
	d.AntialiasedLineEnable = key.antialiasedLineEnable;
	ID3D11RasterizerState* state = nullptr;
	if (!m_device || FAILED(m_device->CreateRasterizerState(&d, &state)))
		return nullptr;
	m_cache[key] = state;
	return state;
}

void CD3D11RasterStateCache::Apply()
{ 
	if (!m_context || !m_dirty)
		return;
	ID3D11RasterizerState* s = GetCurrentState();
	m_context->RSSetState(s);
	m_dirty = false;
}

