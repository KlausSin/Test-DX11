#include "pch.h"
#include "EntityWorld.h"

CEntityWorld::CEntityWorld()
{
}

CEntityWorld::~CEntityWorld()
{
	m_registry.clear();
}

EntityHandle CEntityWorld::CreateEntity()
{
	return m_registry.create();
}

void CEntityWorld::DestroyEntity(EntityHandle entity)
{
	if (!m_registry.valid(entity))
		return;

	DestroyEntityComponents(entity);

	m_registry.destroy(entity);
}

void CEntityWorld::DestroyEntityComponents(EntityHandle entity)
{
	if (!m_registry.valid(entity))
		return;

	for (auto&& storagePair : m_registry.storage())
	{
		auto& storage = storagePair.second;

		if (!storage.contains(entity))
			continue;

		if (void* raw = storage.value(entity))
			static_cast<CEntityComponent*>(raw)->Destroy();

		storage.remove(entity);
	}
}

bool CEntityWorld::IsValid(EntityHandle entity) const
{
	return m_registry.valid(entity);
}

CEntityWorld::Registry& CEntityWorld::GetRegistry()
{
	return m_registry;
}

const CEntityWorld::Registry& CEntityWorld::GetRegistry() const
{
	return m_registry;
}