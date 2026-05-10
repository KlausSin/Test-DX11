#include "D3D11DepthStencilStateCache.h"
#include <EterBase/Debug.h>
#include <assert.h>

SD3D11DepthStencilStateKey::SD3D11DepthStencilStateKey()
{
	std::memset(this, 0, sizeof(*this));
	depthEnable = TRUE;
	depthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	depthFunc = D3D11_COMPARISON_LESS_EQUAL;
	stencilEnable = FALSE;
	stencilReadMask = D3D11_DEFAULT_STENCIL_READ_MASK;
	stencilWriteMask = D3D11_DEFAULT_STENCIL_WRITE_MASK;
	frontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	frontFace.StencilDepthFailOp = D3D11_STENCIL_OP_KEEP;
	frontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	frontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;
	backFace = frontFace;
}

bool SD3D11DepthStencilStateKey::operator==(const SD3D11DepthStencilStateKey& rhs) const
{ 
	return std::memcmp(this, &rhs, sizeof(*this)) == 0; 
}

CD3D11DepthStencilStateCache::CD3D11DepthStencilStateCache() : m_device(nullptr), m_context(nullptr), m_stencilRef(0), m_dirty(true)
{
}

CD3D11DepthStencilStateCache::~CD3D11DepthStencilStateCache()
{ 
	Destroy();
}

void CD3D11DepthStencilStateCache::Initialize(ID3D11Device* device, ID3D11DeviceContext* context)
{
	m_device = device; 
	m_context = context;
	m_dirty = true; 
}

void CD3D11DepthStencilStateCache::Destroy()
{ 
	for (auto& it : m_cache)
		SAFE_RELEASE_D3D11(it.second);
		m_cache.clear();
		m_stack.clear();
		m_device = nullptr;
		m_context = nullptr;
		m_dirty = true;
}

void CD3D11DepthStencilStateCache::ResetDefault() 
{ 
	m_key = SD3D11DepthStencilStateKey(); 
	m_stencilRef = 0;
	m_dirty = true;
}

void CD3D11DepthStencilStateCache::ForceDirty()
{ 
	m_dirty = true; 
}

bool CD3D11DepthStencilStateCache::IsDirty() const 
{ 
	return m_dirty;
}

bool CD3D11DepthStencilStateCache::CanRestore() const
{ 
	return !m_stack.empty();
}

void CD3D11DepthStencilStateCache::Push()
{
	SD3D11DepthStencilRuntimeState s;
	s.key = m_key;
	s.stencilRef = m_stencilRef;
	m_stack.push_back(s);
}

bool CD3D11DepthStencilStateCache::Restore()
{ 
#ifdef _DEBUG
	if (m_stack.empty())
	{
		Tracef("CD3D11DepthStencilStateCache::Restore - state was not pushed\n");
		assert(!"CD3D11DepthStencilStateCache::Restore - state was not pushed");
		return false;
	}
#else
	if (m_stack.empty())
		return false;
#endif

	SD3D11DepthStencilRuntimeState s = m_stack.back();
	m_stack.pop_back(); 
	m_key = s.key; 
	m_stencilRef = s.stencilRef;
	m_dirty = true; 
	return true;
}

void CD3D11DepthStencilStateCache::ClearStack()
{ 
	m_stack.clear();
}

void CD3D11DepthStencilStateCache::SetDepthEnable(BOOL v) 
{ 
	if (m_key.depthEnable != v) 
	{
		m_key.depthEnable = v; 
		m_dirty = true;
	}
}

void CD3D11DepthStencilStateCache::SetDepthWriteEnable(BOOL v)
{ 
	SetDepthWriteMask(v ? D3D11_DEPTH_WRITE_MASK_ALL : D3D11_DEPTH_WRITE_MASK_ZERO);
}

void CD3D11DepthStencilStateCache::SetDepthWriteMask(D3D11_DEPTH_WRITE_MASK v)
{
	if (m_key.depthWriteMask != v)
	{
		m_key.depthWriteMask = v;
		m_dirty = true;
	}
}

void CD3D11DepthStencilStateCache::SetDepthFunc(D3D11_COMPARISON_FUNC v)
{ 
	if (m_key.depthFunc != v)
	{
		m_key.depthFunc = v;
		m_dirty = true;
	} 
}

void CD3D11DepthStencilStateCache::SetStencilEnable(BOOL v)
{ 
	if (m_key.stencilEnable != v)
	{ 
		m_key.stencilEnable = v;
		m_dirty = true;
	} 
}

void CD3D11DepthStencilStateCache::SetStencilReadMask(UINT8 v)
{
	if (m_key.stencilReadMask != v) 
	{
		m_key.stencilReadMask = v;
		m_dirty = true;
	} 
}

void CD3D11DepthStencilStateCache::SetStencilWriteMask(UINT8 v)
{ 
	if (m_key.stencilWriteMask != v)
	{ 
		m_key.stencilWriteMask = v; 
		m_dirty = true;
	}
}

void CD3D11DepthStencilStateCache::SetStencilMasks(UINT8 a, UINT8 b)
{ 
	SetStencilReadMask(a);
	SetStencilWriteMask(b);
}

void CD3D11DepthStencilStateCache::SetStencilRef(UINT v)
{
	if (m_stencilRef != v)
	{
		m_stencilRef = v;
		m_dirty = true;
	} 
}

void CD3D11DepthStencilStateCache::SetFrontFace(const D3D11_DEPTH_STENCILOP_DESC& v)
{
	if (std::memcmp(&m_key.frontFace, &v, sizeof(v)) != 0)
	{
		m_key.frontFace = v;
		m_dirty = true; 
	} 
}

void CD3D11DepthStencilStateCache::SetBackFace(const D3D11_DEPTH_STENCILOP_DESC& v)
{ 
	if (std::memcmp(&m_key.backFace, &v, sizeof(v)) != 0)
	{ 
		m_key.backFace = v; 
		m_dirty = true;
	} 
}

void CD3D11DepthStencilStateCache::SetFrontStencilOps(D3D11_STENCIL_OP a, D3D11_STENCIL_OP b, D3D11_STENCIL_OP c, D3D11_COMPARISON_FUNC f)
{ 
	D3D11_DEPTH_STENCILOP_DESC d = {};
	d.StencilFailOp = a; 
	d.StencilDepthFailOp = b; 
	d.StencilPassOp = c; 
	d.StencilFunc = f; 
	SetFrontFace(d);
}

void CD3D11DepthStencilStateCache::SetBackStencilOps(D3D11_STENCIL_OP a, D3D11_STENCIL_OP b, D3D11_STENCIL_OP c, D3D11_COMPARISON_FUNC f) 
{
	D3D11_DEPTH_STENCILOP_DESC d = {};
	d.StencilFailOp = a;
	d.StencilDepthFailOp = b;
	d.StencilPassOp = c;
	d.StencilFunc = f; 
	SetBackFace(d);
}

void CD3D11DepthStencilStateCache::SetStencilOps(D3D11_STENCIL_OP a, D3D11_STENCIL_OP b, D3D11_STENCIL_OP c, D3D11_COMPARISON_FUNC f)
{ 
	SetFrontStencilOps(a, b, c, f); 
	SetBackStencilOps(a, b, c, f);
}

void CD3D11DepthStencilStateCache::SetKey(const SD3D11DepthStencilStateKey& key) 
{ 
	if (!(m_key == key)) 
	{ 
		m_key = key; 
		m_dirty = true; 
	}
}

BOOL CD3D11DepthStencilStateCache::GetDepthEnable() const
{ 
	return m_key.depthEnable;
}

BOOL CD3D11DepthStencilStateCache::GetDepthWriteEnable() const 
{ 
	return m_key.depthWriteMask == D3D11_DEPTH_WRITE_MASK_ALL; 
}

D3D11_DEPTH_WRITE_MASK CD3D11DepthStencilStateCache::GetDepthWriteMask() const 
{ 
	return m_key.depthWriteMask;
}

D3D11_COMPARISON_FUNC CD3D11DepthStencilStateCache::GetDepthFunc() const 
{
	return m_key.depthFunc; 
}

BOOL CD3D11DepthStencilStateCache::GetStencilEnable() const 
{
	return m_key.stencilEnable;
}

UINT8 CD3D11DepthStencilStateCache::GetStencilReadMask() const 
{ 
	return m_key.stencilReadMask;
}

UINT8 CD3D11DepthStencilStateCache::GetStencilWriteMask() const 
{ 
	return m_key.stencilWriteMask;
}

UINT CD3D11DepthStencilStateCache::GetStencilRef() const 
{
	return m_stencilRef;
}

const D3D11_DEPTH_STENCILOP_DESC& CD3D11DepthStencilStateCache::GetFrontFace() const 
{ 
	return m_key.frontFace;
}

const D3D11_DEPTH_STENCILOP_DESC& CD3D11DepthStencilStateCache::GetBackFace() const 
{ 
	return m_key.backFace;
}

const SD3D11DepthStencilStateKey& CD3D11DepthStencilStateCache::GetKey() const 
{ 
	return m_key;
}

ID3D11DepthStencilState* CD3D11DepthStencilStateCache::GetCurrentState() 
{
	return GetOrCreate(m_key); 
}

ID3D11DepthStencilState* CD3D11DepthStencilStateCache::GetOrCreate(const SD3D11DepthStencilStateKey& key)
{
	auto it = m_cache.find(key);
	if (it != m_cache.end())
		return it->second;
	D3D11_DEPTH_STENCIL_DESC d = {};
	d.DepthEnable = key.depthEnable;
	d.DepthWriteMask = key.depthWriteMask;
	d.DepthFunc = key.depthFunc;
	d.StencilEnable = key.stencilEnable;
	d.StencilReadMask = key.stencilReadMask;
	d.StencilWriteMask = key.stencilWriteMask;
	d.FrontFace = key.frontFace;
	d.BackFace = key.backFace;
	ID3D11DepthStencilState* state = nullptr;
	if (!m_device || FAILED(m_device->CreateDepthStencilState(&d, &state)))
		return nullptr;
	m_cache[key] = state;
	return state;
}

void CD3D11DepthStencilStateCache::Apply()
{
	if (!m_context || !m_dirty)
		return;
	ID3D11DepthStencilState* s = GetCurrentState();
	m_context->OMSetDepthStencilState(s, m_stencilRef);
	m_dirty = false;
}

