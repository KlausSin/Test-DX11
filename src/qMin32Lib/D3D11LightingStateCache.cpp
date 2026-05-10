#include "pch.h"
#include "D3D11LightingStateCache.h"
#include "DxManager.h"
#include "ConstantBufferManager.h"
#include "EterLib/GrpBase.h"
#include <EterBase/Debug.h>

SD3D11LightingStateKey::SD3D11LightingStateKey()
{
	lightingEnable = false;
	ambient = 0x00000000;
	material = D3DMATERIAL11();

	for (UINT i = 0; i < 8; ++i)
		lights[i] = D3DLIGHT11();
}

bool SD3D11LightingStateKey::operator==(const SD3D11LightingStateKey& rhs) const
{
	return std::memcmp(this, &rhs, sizeof(*this)) == 0;
}

CD3D11LightingStateCache::CD3D11LightingStateCache() : m_dirty(true)
{
}

void CD3D11LightingStateCache::ResetDefault()
{
	m_key = SD3D11LightingStateKey();
	m_dirty = true;
	ApplyKey();
}

void CD3D11LightingStateCache::ForceDirty()
{
	m_dirty = true;
	ApplyKey();
}

bool CD3D11LightingStateCache::IsDirty() const
{
	return m_dirty;
}

bool CD3D11LightingStateCache::CanRestore() const
{
	return !m_stack.empty();
}

void CD3D11LightingStateCache::Push()
{
	m_stack.push_back(m_key);
}

bool CD3D11LightingStateCache::Restore()
{
#ifdef _DEBUG
	if (m_stack.empty())
	{
		Tracef("CD3D11LightingStateCache::Restore - state was not pushed\n");
		assert(!"CD3D11LightingStateCache::Restore - state was not pushed");
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

void CD3D11LightingStateCache::ClearStack()
{
	m_stack.clear();
}

void CD3D11LightingStateCache::SetLightingEnable(bool value)
{
	if (m_key.lightingEnable != value)
	{
		m_key.lightingEnable = value;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetLightingEnable(value);
}

void CD3D11LightingStateCache::SetAmbient(DWORD color)
{
	if (m_key.ambient != color)
	{
		m_key.ambient = color;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetAmbient(color);
}

void CD3D11LightingStateCache::SetMaterial(const D3DMATERIAL11& material)
{
	if (std::memcmp(&m_key.material, &material, sizeof(material)) != 0)
	{
		m_key.material = material;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetMaterial(&material);
}

void CD3D11LightingStateCache::SetLight(UINT index, const D3DLIGHT11& light)
{
	if (!IsValidLight(index))
		return;

	if (std::memcmp(&m_key.lights[index], &light, sizeof(light)) != 0)
	{
		m_key.lights[index] = light;
		m_dirty = true;
	}

	_mgr->GetCbMgr()->SetLight(index, &light);
}

void CD3D11LightingStateCache::SetKey(const SD3D11LightingStateKey& key)
{
	if (!(m_key == key))
	{
		m_key = key;
		m_dirty = true;
	}

	ApplyKey();
}

bool CD3D11LightingStateCache::GetLightingEnable() const
{
	return m_key.lightingEnable;
}

DWORD CD3D11LightingStateCache::GetAmbient() const
{
	return m_key.ambient;
}

const D3DMATERIAL11& CD3D11LightingStateCache::GetMaterial() const
{
	return m_key.material;
}

const D3DLIGHT11& CD3D11LightingStateCache::GetLight(UINT index) const
{
	static D3DLIGHT11 defaultLight;

	if (!IsValidLight(index))
		return defaultLight;

	return m_key.lights[index];
}

const SD3D11LightingStateKey& CD3D11LightingStateCache::GetKey() const
{
	return m_key;
}

bool CD3D11LightingStateCache::IsValidLight(UINT index) const
{
	return index < 8;
}

void CD3D11LightingStateCache::ApplyKey()
{
	auto cb = _mgr->GetCbMgr();

	cb->SetLightingEnable(m_key.lightingEnable);
	cb->SetAmbient(m_key.ambient);
	cb->SetMaterial(&m_key.material);

	for (UINT i = 0; i < 8; ++i)
		cb->SetLight(i, &m_key.lights[i]);
}
