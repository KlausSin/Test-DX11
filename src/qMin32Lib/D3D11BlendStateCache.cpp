#include "D3D11BlendStateCache.h"

SD3D11BlendStateKey::SD3D11BlendStateKey()
{
	std::memset(this, 0, sizeof(*this));
	alphaToCoverageEnable = FALSE;
	independentBlendEnable = FALSE;
	for (UINT i = 0; i < 8; ++i)
	{
		renderTarget[i].BlendEnable = FALSE;
		renderTarget[i].SrcBlend = D3D11_BLEND_SRC_ALPHA;
		renderTarget[i].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
		renderTarget[i].BlendOp = D3D11_BLEND_OP_ADD;
		renderTarget[i].SrcBlendAlpha = D3D11_BLEND_ONE;
		renderTarget[i].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
		renderTarget[i].BlendOpAlpha = D3D11_BLEND_OP_ADD;
		renderTarget[i].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
	}
}

bool SD3D11BlendStateKey::operator==(const SD3D11BlendStateKey& rhs) const 
{ 
	return std::memcmp(this, &rhs, sizeof(*this)) == 0;
}

CD3D11BlendStateCache::CD3D11BlendStateCache() : m_device(nullptr), m_context(nullptr), m_sampleMask(0xFFFFFFFF), m_dirty(true)
{ 
	m_blendFactor[0] = m_blendFactor[1] = m_blendFactor[2] = m_blendFactor[3] = 0.0f;
}

CD3D11BlendStateCache::~CD3D11BlendStateCache()
{ 
	Destroy(); 
}

void CD3D11BlendStateCache::Initialize(ID3D11Device* device, ID3D11DeviceContext* context) 
{ 
	m_device = device;
	m_context = context;
	m_dirty = true;
}

void CD3D11BlendStateCache::Destroy() 
{ 
	for (auto& it : m_cache)
		SAFE_RELEASE_D3D11(it.second); 
	m_cache.clear();
	m_stack.clear();
	m_device = nullptr; 
	m_context = nullptr;
	m_dirty = true; 
}

void CD3D11BlendStateCache::ResetDefault()
{ 
	m_key = SD3D11BlendStateKey(); 
	m_blendFactor[0] = m_blendFactor[1] = m_blendFactor[2] = m_blendFactor[3] = 0.0f;
	m_sampleMask = 0xFFFFFFFF;
	m_dirty = true;
}

void CD3D11BlendStateCache::ForceDirty() 
{ 
	m_dirty = true; 
}

bool CD3D11BlendStateCache::IsDirty() const 
{ 
	return m_dirty;
}

bool CD3D11BlendStateCache::CanRestore() const 
{ 
	return !m_stack.empty(); 
}

void CD3D11BlendStateCache::Push() 
{
	SD3D11BlendRuntimeState s;
	s.key = m_key;
	s.blendFactor[0] = m_blendFactor[0];
	s.blendFactor[1] = m_blendFactor[1];
	s.blendFactor[2] = m_blendFactor[2];
	s.blendFactor[3] = m_blendFactor[3];
	s.sampleMask = m_sampleMask;
	m_stack.push_back(s);
}

bool CD3D11BlendStateCache::Restore() 
{ 
	if (m_stack.empty())
		return false;
	SD3D11BlendRuntimeState s = m_stack.back();
	m_stack.pop_back();
	m_key = s.key;
	m_blendFactor[0] = s.blendFactor[0];
	m_blendFactor[1] = s.blendFactor[1]; 
	m_blendFactor[2] = s.blendFactor[2]; 
	m_blendFactor[3] = s.blendFactor[3];
	m_sampleMask = s.sampleMask; 
	m_dirty = true; 
	return true;
}

void CD3D11BlendStateCache::ClearStack()
{ 
	m_stack.clear();
}

bool CD3D11BlendStateCache::IsValidRT(UINT i) const 
{ 
	return i < 8;
}

void CD3D11BlendStateCache::SetAlphaToCoverageEnable(BOOL v)
{ 
	if (m_key.alphaToCoverageEnable != v) 
	{
		m_key.alphaToCoverageEnable = v; 
		m_dirty = true;
	} 
}

void CD3D11BlendStateCache::SetIndependentBlendEnable(BOOL v) 
{ 
	if (m_key.independentBlendEnable != v)
	{
		m_key.independentBlendEnable = v;
		m_dirty = true;
	}
}


void CD3D11BlendStateCache::SetBlendFactor(FLOAT r, FLOAT g, FLOAT b, FLOAT a) 
{
	if (m_blendFactor[0] != r || m_blendFactor[1] != g || m_blendFactor[2] != b || m_blendFactor[3] != a)
	{ 
		m_blendFactor[0] = r;
		m_blendFactor[1] = g;
		m_blendFactor[2] = b;
		m_blendFactor[3] = a;
		m_dirty = true; 
	}
}

void CD3D11BlendStateCache::SetBlendEnable(BOOL v)
{
	if (m_key.renderTarget[0].BlendEnable != v)
	{
		m_key.renderTarget[0].BlendEnable = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetSrcBlend(D3D11_BLEND v)
{
	if (m_key.renderTarget[0].SrcBlend != v)
	{
		m_key.renderTarget[0].SrcBlend = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetDestBlend(D3D11_BLEND v)
{
	if (m_key.renderTarget[0].DestBlend != v)
	{
		m_key.renderTarget[0].DestBlend = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetBlendOp(D3D11_BLEND_OP v)
{
	if (m_key.renderTarget[0].BlendOp != v)
	{
		m_key.renderTarget[0].BlendOp = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetSrcBlendAlpha(D3D11_BLEND v)
{
	if (m_key.renderTarget[0].SrcBlendAlpha != v)
	{
		m_key.renderTarget[0].SrcBlendAlpha = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetDestBlendAlpha(D3D11_BLEND v)
{
	if (m_key.renderTarget[0].DestBlendAlpha != v)
	{
		m_key.renderTarget[0].DestBlendAlpha = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetBlendOpAlpha(D3D11_BLEND_OP v)
{
	if (m_key.renderTarget[0].BlendOpAlpha != v)
	{
		m_key.renderTarget[0].BlendOpAlpha = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetColorWriteMask(UINT8 v)
{
	if (m_key.renderTarget[0].RenderTargetWriteMask != v)
	{
		m_key.renderTarget[0].RenderTargetWriteMask = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetRenderTarget(const D3D11_RENDER_TARGET_BLEND_DESC& v)
{
	if (std::memcmp(&m_key.renderTarget[0], &v, sizeof(v)) != 0)
	{
		m_key.renderTarget[0] = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetBlendFactorR(FLOAT v) 
{ 
	SetBlendFactor(v, m_blendFactor[1], m_blendFactor[2], m_blendFactor[3]);
}

void CD3D11BlendStateCache::SetBlendFactorG(FLOAT v)
{ 
	SetBlendFactor(m_blendFactor[0], v, m_blendFactor[2], m_blendFactor[3]);
}

void CD3D11BlendStateCache::SetBlendFactorB(FLOAT v)
{
	SetBlendFactor(m_blendFactor[0], m_blendFactor[1], v, m_blendFactor[3]);
}

void CD3D11BlendStateCache::SetBlendFactorA(FLOAT v) 
{ 
	SetBlendFactor(m_blendFactor[0], m_blendFactor[1], m_blendFactor[2], v);
}

void CD3D11BlendStateCache::SetSampleMask(UINT v)
{ 
	if (m_sampleMask != v)
	{
		m_sampleMask = v;
		m_dirty = true;
	}
}

void CD3D11BlendStateCache::SetKey(const SD3D11BlendStateKey& key)
{ 
	if (!(m_key == key))
	{ 
		m_key = key; 
		m_dirty = true;
	}
}

BOOL CD3D11BlendStateCache::GetAlphaToCoverageEnable() const 
{ 
	return m_key.alphaToCoverageEnable;
}

BOOL CD3D11BlendStateCache::GetIndependentBlendEnable() const
{ 
	return m_key.independentBlendEnable;
}

BOOL CD3D11BlendStateCache::GetBlendEnable() const
{
	return m_key.renderTarget[0].BlendEnable;
}

D3D11_BLEND CD3D11BlendStateCache::GetSrcBlend() const
{
	return m_key.renderTarget[0].SrcBlend;
}

D3D11_BLEND CD3D11BlendStateCache::GetDestBlend() const
{
	return m_key.renderTarget[0].DestBlend;
}

D3D11_BLEND_OP CD3D11BlendStateCache::GetBlendOp() const
{
	return m_key.renderTarget[0].BlendOp;
}

D3D11_BLEND CD3D11BlendStateCache::GetSrcBlendAlpha() const
{
	return m_key.renderTarget[0].SrcBlendAlpha;
}

D3D11_BLEND CD3D11BlendStateCache::GetDestBlendAlpha() const
{
	return m_key.renderTarget[0].DestBlendAlpha;
}

D3D11_BLEND_OP CD3D11BlendStateCache::GetBlendOpAlpha() const
{
	return m_key.renderTarget[0].BlendOpAlpha;
}

UINT8 CD3D11BlendStateCache::GetColorWriteMask() const
{
	return m_key.renderTarget[0].RenderTargetWriteMask;
}

const D3D11_RENDER_TARGET_BLEND_DESC& CD3D11BlendStateCache::GetRenderTarget() const
{
	return m_key.renderTarget[0];
}

void CD3D11BlendStateCache::GetBlendFactor(FLOAT outFactor[4]) const
{ 
	if (!outFactor) 
		return; 
	outFactor[0] = m_blendFactor[0];
	outFactor[1] = m_blendFactor[1];
	outFactor[2] = m_blendFactor[2];
	outFactor[3] = m_blendFactor[3];
}

UINT CD3D11BlendStateCache::GetSampleMask() const
{
	return m_sampleMask;
}

const SD3D11BlendStateKey& CD3D11BlendStateCache::GetKey() const 
{ 
	return m_key;
}

ID3D11BlendState* CD3D11BlendStateCache::GetCurrentState()
{
	return GetOrCreate(m_key);
}

ID3D11BlendState* CD3D11BlendStateCache::GetOrCreate(const SD3D11BlendStateKey& key)
{
	auto it = m_cache.find(key);
	if (it != m_cache.end())
		return it->second;
	D3D11_BLEND_DESC d = {};
	d.AlphaToCoverageEnable = key.alphaToCoverageEnable; d.IndependentBlendEnable = key.independentBlendEnable;
	for (UINT i = 0; i < 8; ++i)
		d.RenderTarget[i] = key.renderTarget[i];
	ID3D11BlendState* state = nullptr;
	if (!m_device || FAILED(m_device->CreateBlendState(&d, &state))) return nullptr;
	m_cache[key] = state;
	return state;
}

void CD3D11BlendStateCache::Apply()
{ 
	if (!m_context || !m_dirty)
		return; 
	ID3D11BlendState* s = GetCurrentState();
	m_context->OMSetBlendState(s, m_blendFactor, m_sampleMask);
	m_dirty = false;
}
