#include "pch.h"
#include "AttributeComponent.h"

CAttributeComponent::CAttributeComponent()
	: m_heightInstance(nullptr)
{
	memset(m_portalID, 0, sizeof(m_portalID));
}

CAttributeComponent::~CAttributeComponent()
{
}

void CAttributeComponent::OnCreateInternal()
{
	m_heightInstance = nullptr;

	memset(m_portalID, 0, sizeof(m_portalID));
}

void CAttributeComponent::OnDestroyInternal()
{
	m_heightInstance = nullptr;
}

void CAttributeComponent::SetHeightInstance(CAttributeInstance* instance)
{
	m_heightInstance = instance;
}

CAttributeInstance* CAttributeComponent::GetHeightInstance()
{
	return m_heightInstance;
}

const CAttributeInstance* CAttributeComponent::GetHeightInstance() const
{
	return m_heightInstance;
}

void CAttributeComponent::ClearHeightInstance()
{
	m_heightInstance = nullptr;
}

bool CAttributeComponent::HasHeightInstance() const
{
	return m_heightInstance != nullptr;
}

void CAttributeComponent::SetPortalID(uint32_t index, BYTE id)
{
	if (index >= 8)
		return;

	m_portalID[index] = id;
}

BYTE CAttributeComponent::GetPortalID(uint32_t index) const
{
	if (index >= 8)
		return 0;

	return m_portalID[index];
}

void CAttributeComponent::ClearPortalIDs()
{
	memset(m_portalID, 0, sizeof(m_portalID));
}
