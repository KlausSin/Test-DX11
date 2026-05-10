#include "pch.h"
#include "D3D11TransformStateCache.h"
#include "DxManager.h"
#include "ConstantBufferManager.h"
#include "EterLib/GrpBase.h"

SD3D11TransformStateKey::SD3D11TransformStateKey()
{
	D3DXMatrixIdentity(&world);
	D3DXMatrixIdentity(&view);
	D3DXMatrixIdentity(&projection);

	for (UINT i = 0; i < 2; ++i)
		D3DXMatrixIdentity(&texture[i]);
}

bool SD3D11TransformStateKey::operator==(const SD3D11TransformStateKey& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(*this)) == 0;
}

CD3D11TransformStateCache::CD3D11TransformStateCache() : m_dirty(true)
{
}

void CD3D11TransformStateCache::ResetDefault()
{
	m_key = SD3D11TransformStateKey();
	m_dirty = true;
	ApplyKey();
}

void CD3D11TransformStateCache::ForceDirty()
{
	m_dirty = true;
}

bool CD3D11TransformStateCache::IsDirty() const
{
	return m_dirty;
}

bool CD3D11TransformStateCache::CanRestore() const
{
	return !m_stack.empty();
}

void CD3D11TransformStateCache::Push()
{
	m_stack.push_back(m_key);
}

bool CD3D11TransformStateCache::Restore()
{
	if (m_stack.empty())
		return false;

	m_key = m_stack.back();
	m_stack.pop_back();

	m_dirty = true;
	ApplyKey();

	return true;
}

void CD3D11TransformStateCache::ClearStack()
{
	m_stack.clear();
}

void CD3D11TransformStateCache::SetWorld(const D3DXMATRIX& value)
{
	if (std::memcmp(&m_key.world, &value, sizeof(value)) != 0)
	{
		m_key.world = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetWorldMatrix(value);
}

void CD3D11TransformStateCache::SetView(const D3DXMATRIX& value)
{
	if (std::memcmp(&m_key.view, &value, sizeof(value)) != 0)
	{
		m_key.view = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetViewMatrix(value);
}

void CD3D11TransformStateCache::SetProjection(const D3DXMATRIX& value)
{
	if (std::memcmp(&m_key.projection, &value, sizeof(value)) != 0)
	{
		m_key.projection = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetProjMatrix(value);
}

void CD3D11TransformStateCache::SetTexture0(const D3DXMATRIX& value)
{
	_mgr->GetCbMgr()->SetTexTransform(0, value);
}

void CD3D11TransformStateCache::SetTexture1(const D3DXMATRIX& value)
{
	_mgr->GetCbMgr()->SetTexTransform(1, value);
}

void CD3D11TransformStateCache::SetKey(const SD3D11TransformStateKey& key)
{
	if (!(m_key == key))
	{
		m_key = key;
		m_dirty = true;
	}

	ApplyKey();
}

const D3DXMATRIX& CD3D11TransformStateCache::GetWorld() const
{
	return m_key.world;
}

const D3DXMATRIX& CD3D11TransformStateCache::GetView() const
{
	return m_key.view;
}

const D3DXMATRIX& CD3D11TransformStateCache::GetProjection() const
{
	return m_key.projection;
}

const D3DXMATRIX& CD3D11TransformStateCache::GetTexture0() const
{
	return m_key.texture[0];
}

const D3DXMATRIX& CD3D11TransformStateCache::GetTexture1() const
{
	return m_key.texture[1];
}

const SD3D11TransformStateKey& CD3D11TransformStateCache::GetKey() const
{
	return m_key;
}

void CD3D11TransformStateCache::ApplyKey()
{
	auto cb = _mgr->GetCbMgr();

	cb->SetWorldMatrix(m_key.world);
	cb->SetViewMatrix(m_key.view);
	cb->SetProjMatrix(m_key.projection);

	for (UINT i = 0; i < 2; ++i)
		cb->SetTexTransform(i, m_key.texture[i]);
}
