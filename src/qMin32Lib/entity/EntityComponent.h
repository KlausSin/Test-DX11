#pragma once

#include "EntityTypes.h"
#include <DirectXMath.h>
#include <vector>

class CEntity;
class CEntityComponent
{
public:
	virtual ~CEntityComponent() = default;

	virtual void OnCreateInternal() {}
	virtual void OnDestroyInternal() {}

	void Create(CEntity* owner);
	void Destroy();

	CEntity* GetOwner() const;

protected:
	CEntity* m_owner = nullptr;
};
