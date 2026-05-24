#include "pch.h"
#include "EntityComponent.h"

void CEntityComponent::Create(CEntity* owner)
{
	m_owner = owner;
	OnCreateInternal();
}

void CEntityComponent::Destroy()
{
	OnDestroyInternal();
	m_owner = nullptr;
}

CEntity* CEntityComponent::GetOwner() const
{
	return m_owner;
}