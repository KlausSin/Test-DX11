#pragma once

#include "EntityComponent.h"
#include "EterLib/AttributeInstance.h"

class CAttributeComponent : public CEntityComponent
{
public:
	CAttributeComponent();
	virtual ~CAttributeComponent();

	void OnCreateInternal() override;
	void OnDestroyInternal() override;

	void SetHeightInstance(CAttributeInstance* instance);
	CAttributeInstance* GetHeightInstance();
	const CAttributeInstance* GetHeightInstance() const;

	void ClearHeightInstance();

	bool HasHeightInstance() const;

	void SetPortalID(uint32_t index, BYTE id);
	BYTE GetPortalID(uint32_t index) const;

	void ClearPortalIDs();

private:
	CAttributeInstance* m_heightInstance;
	BYTE m_portalID[8];
};