#pragma once

#include "EntityComponent.h"
#include "EterLib/CollisionData.h"

class CCollisionComponent : public CEntityComponent
{
public:
	CCollisionComponent();
	virtual ~CCollisionComponent();

	void OnCreateInternal() override;
	void OnDestroyInternal() override;

	void Clear();

	void Add(const CStaticCollisionData* data, const XMFLOAT4X4* matrix);

	bool CollisionDynamicSphere(const CDynamicSphereInstance& sphere) const;
	bool MovementCollisionDynamicSphere(const CDynamicSphereInstance& sphere) const;
	XMFLOAT3 GetCollisionMovementAdjust(const CDynamicSphereInstance& sphere) const;

	DWORD GetCount() const;

	CBaseCollisionInstance* GetData(DWORD index);
	const CBaseCollisionInstance* GetData(DWORD index) const;

	CCollisionInstanceVector& GetInstances();
	const CCollisionInstanceVector& GetInstances() const;

private:
	CCollisionInstanceVector m_instances;
};
