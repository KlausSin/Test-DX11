#pragma once

// Collision Detection
typedef struct SSphereData
{
	XMFLOAT3 v3Position;
	float fRadius;
} TSphereData;

typedef struct SPlaneData
{
	XMFLOAT3 v3Position;
	XMFLOAT3 v3Normal;

	XMFLOAT3 v3QuadPosition[4];
	XMFLOAT3 v3InsideVector[4];
} TPlaneData;

typedef struct SAABBData
{
	XMFLOAT3 v3Min;
	XMFLOAT3 v3Max;

} TAABBData;

typedef struct SOBBData
{
	XMFLOAT3 v3Min;
	XMFLOAT3 v3Max;
	XMFLOAT4X4 matRot;

} TOBBData;

typedef struct SCylinderData
{
	XMFLOAT3 v3Position;
	float fRadius;
	float fHeight;
} TCylinderData;

enum ECollisionType
{
	COLLISION_TYPE_PLANE,
	COLLISION_TYPE_BOX,
	COLLISION_TYPE_SPHERE,
	COLLISION_TYPE_CYLINDER,
	COLLISION_TYPE_AABB,
	COLLISION_TYPE_OBB,
};

struct CDynamicSphereInstance
{
	XMFLOAT3 v3Position;
	XMFLOAT3 v3LastPosition;

	float fRadius;
};

class CStaticCollisionData
{
public:
	uint32_t dwType;
	char szName[32 + 1];

	XMFLOAT3 v3Position;
	float fDimensions[3];
	XMFLOAT4 quatRotation;
};

void DestroyCollisionInstanceSystem();

typedef std::vector<CStaticCollisionData> CStaticCollisionDataVector;

/////////////////////////////////////////////
// Base
class CBaseCollisionInstance
{
public:
	virtual void Render(D3D11_FILL_MODE D3D11_FILL_MODE = D3D11_FILL_SOLID) = 0;

	bool MovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
	{
		return OnMovementCollisionDynamicSphere(s);
	}
	bool CollisionDynamicSphere(const CDynamicSphereInstance& s) const
	{
		return OnCollisionDynamicSphere(s);
	}


	XMFLOAT3 GetCollisionMovementAdjust(const CDynamicSphereInstance& s) const
	{
		return OnGetCollisionMovementAdjust(s);
	}

	void Destroy();

	static CBaseCollisionInstance* BuildCollisionInstance(const CStaticCollisionData* c_pCollisionData, const XMFLOAT4X4* pMat);

protected:
	virtual XMFLOAT3 OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const = 0;
	virtual bool OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const = 0;
	virtual bool OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const = 0;
	virtual void OnDestroy() = 0;
};

/////////////////////////////////////////////
// Sphere
class CSphereCollisionInstance : public CBaseCollisionInstance
{
public:
	TSphereData& GetAttribute();
	const TSphereData& GetAttribute() const;
	virtual void Render(D3D11_FILL_MODE D3D11_FILL_MODE = D3D11_FILL_SOLID);

protected:
	void OnDestroy();
	bool OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual bool OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual XMFLOAT3 OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const;

protected:
	TSphereData m_attribute;
};

/////////////////////////////////////////////
// Plane
class CPlaneCollisionInstance : public CBaseCollisionInstance
{
public:
	TPlaneData& GetAttribute();
	const TPlaneData& GetAttribute() const;
	virtual void Render(D3D11_FILL_MODE D3D11_FILL_MODE = D3D11_FILL_SOLID);

protected:
	void OnDestroy();
	bool OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual bool OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual XMFLOAT3 OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const;

protected:
	TPlaneData m_attribute;
};

/////////////////////////////////////////////
// AABB (Aligned Axis Bounding Box)
class CAABBCollisionInstance : public CBaseCollisionInstance
{
public:
	TAABBData& GetAttribute();
	const TAABBData& GetAttribute() const;
	virtual void Render(D3D11_FILL_MODE D3D11_FILL_MODE = D3D11_FILL_SOLID);

protected:
	void OnDestroy();
	bool OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual bool OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual XMFLOAT3 OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const;

protected:
	TAABBData m_attribute;
};

/////////////////////////////////////////////
// OBB
class COBBCollisionInstance : public CBaseCollisionInstance
{
public:
	TOBBData& GetAttribute();
	const TOBBData& GetAttribute() const;
	virtual void Render(D3D11_FILL_MODE D3D11_FILL_MODE = D3D11_FILL_SOLID);

protected:
	void OnDestroy();
	bool OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual bool OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual XMFLOAT3 OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const;

protected:
	TOBBData m_attribute;
};

/////////////////////////////////////////////
// Cylinder
class CCylinderCollisionInstance : public CBaseCollisionInstance
{
public:
	TCylinderData& GetAttribute();
	const TCylinderData& GetAttribute() const;
	virtual void Render(D3D11_FILL_MODE D3D11_FILL_MODE = D3D11_FILL_SOLID);

protected:
	void OnDestroy();
	bool OnMovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual bool OnCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
	virtual XMFLOAT3 OnGetCollisionMovementAdjust(const CDynamicSphereInstance& s) const;

	bool CollideCylinderVSDynamicSphere(const TCylinderData& c_rattribute, const CDynamicSphereInstance& s) const;

protected:
	TCylinderData m_attribute;
};

typedef std::vector<CSphereCollisionInstance> CSphereCollisionInstanceVector;
typedef std::vector<CDynamicSphereInstance> CDynamicSphereInstanceVector;
typedef std::vector<CBaseCollisionInstance*> CCollisionInstanceVector;
