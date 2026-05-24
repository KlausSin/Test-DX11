#include "pch.h"
#include "CollisionComponent.h"
#include <EterBase/Debug.h>

CCollisionComponent::CCollisionComponent()
{
}

CCollisionComponent::~CCollisionComponent()
{
	Clear();
}

void CCollisionComponent::OnCreateInternal()
{
}

void CCollisionComponent::OnDestroyInternal()
{
	Clear();
}

void CCollisionComponent::Clear()
{
	for (const auto& instance : m_instances)
	{
		if (instance)
			instance->Destroy();
	}

	m_instances.clear();
}

void CCollisionComponent::Add(const CStaticCollisionData* data, const XMFLOAT4X4* matrix)
{
	auto instance = CBaseCollisionInstance::BuildCollisionInstance(data, matrix);
	if (!instance)
	{
		TraceError("AddCollision failed type=%u", data ? data->dwType : 999);
		return;
	}

	m_instances.push_back(instance);
}

bool CCollisionComponent::CollisionDynamicSphere(const CDynamicSphereInstance& sphere) const
{
	for (auto instance : m_instances)
	{
		if (instance && instance->CollisionDynamicSphere(sphere))
			return true;
	}

	return false;
}

bool CCollisionComponent::MovementCollisionDynamicSphere(const CDynamicSphereInstance& sphere) const
{
	for (auto instance : m_instances)
	{
		if (instance && instance->MovementCollisionDynamicSphere(sphere))
			return true;
	}

	return false;
}

XMFLOAT3 CCollisionComponent::GetCollisionMovementAdjust(const CDynamicSphereInstance& sphere) const
{
	for (auto instance : m_instances)
	{
		if (instance && instance->MovementCollisionDynamicSphere(sphere))
			return instance->GetCollisionMovementAdjust(sphere);
	}

	return XMFLOAT3(0.0f, 0.0f, 0.0f);
}

DWORD CCollisionComponent::GetCount() const
{
	return static_cast<DWORD>(m_instances.size());
}

CBaseCollisionInstance* CCollisionComponent::GetData(DWORD index)
{
	if (index >= m_instances.size())
		return nullptr;

	return m_instances[index].get();
}

const CBaseCollisionInstance* CCollisionComponent::GetData(DWORD index) const
{
	if (index >= m_instances.size())
		return nullptr;

	return m_instances[index].get();
}

CCollisionInstanceVector& CCollisionComponent::GetInstances()
{
	return m_instances;
}

const CCollisionInstanceVector& CCollisionComponent::GetInstances() const
{
	return m_instances;
}
