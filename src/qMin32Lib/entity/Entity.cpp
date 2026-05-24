#include "pch.h"
#include "Entity.h"

CEntityWorld CEntity::ms_world;

CEntity::CEntity()
	: m_entity(entt::null)
{
}

CEntity::~CEntity()
{
	DestroyEntity();
}

void CEntity::CreateEntity()
{
	EnsureEntity();
}

void CEntity::DestroyEntity()
{
	if (!ms_world.IsValid(m_entity))
		return;

	OnDestroyInternal();
	ms_world.DestroyEntity(m_entity);
	m_entity = entt::null;
}

void CEntity::EnsureEntity()
{
	if (ms_world.IsValid(m_entity))
		return;

	m_entity = ms_world.CreateEntity();
	OnCreateInternal();
}

bool CEntity::IsEntityValid() const
{
	return ms_world.IsValid(m_entity);
}

EntityHandle CEntity::GetEntityHandle() const
{
	return m_entity;
}

CEntityWorld& CEntity::GetEntityWorld()
{
	return ms_world;
}
