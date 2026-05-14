#include "pch.h"
#include "D3D11TransformStateCache.h"
#include "DxManager.h"
#include "ConstantBufferManager.h"
#include "EterLib/GrpBase.h"
#include <EterBase/Debug.h>

SD3D11TransformStateKey::SD3D11TransformStateKey()
{
	XMStoreFloat4x4(&world, XMMatrixIdentity());
	XMStoreFloat4x4(&view, XMMatrixIdentity());
	XMStoreFloat4x4(&projection, XMMatrixIdentity());

	for (UINT i = 0; i < 2; ++i)
		XMStoreFloat4x4(&texture[i], XMMatrixIdentity());
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
#ifdef _DEBUG
	if (m_stack.empty())
	{
		Tracef("CD3D11TransformStateCache::Restore - state was not pushed\n");
		assert(!"CD3D11TransformStateCache::Restore - state was not pushed");
		return false;
	}
#else
	if (m_stack.empty())
		return false;
#endif


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

void CD3D11TransformStateCache::SetWorld(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.world, &value, sizeof(value)) != 0)
	{
		m_key.world = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetWorldMatrix(value);
}

void CD3D11TransformStateCache::SetView(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.view, &value, sizeof(value)) != 0)
	{
		m_key.view = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetViewMatrix(value);
}

void CD3D11TransformStateCache::SetProjection(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.projection, &value, sizeof(value)) != 0)
	{
		m_key.projection = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetProjMatrix(value);
}

void CD3D11TransformStateCache::SetTexture0(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.texture[0], &value, sizeof(value)) != 0)
	{
		m_key.texture[0] = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetTexTransform(0, value);
}

void CD3D11TransformStateCache::SetTexture1(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.texture[1], &value, sizeof(value)) != 0)
	{
		m_key.texture[1] = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetTexTransform(1, value);
}

void CD3D11TransformStateCache::SetTexture2(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.texture[2], &value, sizeof(value)) != 0)
	{
		m_key.texture[2] = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetTexTransform(2, value);
}

void CD3D11TransformStateCache::SetTexture3(const XMFLOAT4X4& value)
{
	if (std::memcmp(&m_key.texture[3], &value, sizeof(value)) != 0)
	{
		m_key.texture[3] = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetTexTransform(3, value);
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

const XMFLOAT4X4& CD3D11TransformStateCache::GetWorld() const
{
	return m_key.world;
}

const XMFLOAT4X4& CD3D11TransformStateCache::GetView() const
{
	return m_key.view;
}

const XMFLOAT4X4& CD3D11TransformStateCache::GetProjection() const
{
	return m_key.projection;
}

const XMFLOAT4X4& CD3D11TransformStateCache::GetTexture0() const
{
	return m_key.texture[0];
}

const XMFLOAT4X4& CD3D11TransformStateCache::GetTexture1() const
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
