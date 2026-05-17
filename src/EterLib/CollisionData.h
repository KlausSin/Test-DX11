#pragma once
#include <vector>
#include <cstdint>
#include "qMin32Lib/JoltTypes.h"
#include "qMin32Lib/PhysicsObjectComponent.h"
#include <d3d11.h>
#include <DirectXMath.h>
using namespace DirectX;

struct SSphereData { XMFLOAT3 v3Position; float fRadius; };
typedef SSphereData TSphereData;

struct SPlaneData
{
    XMFLOAT3 v3Position;
    XMFLOAT3 v3Normal;
    XMFLOAT3 v3QuadPosition[4];
    XMFLOAT3 v3InsideVector[4];
};
typedef SPlaneData TPlaneData;

struct SAABBData { XMFLOAT3 v3Min; XMFLOAT3 v3Max; };
typedef SAABBData TAABBData;

struct SOBBData { XMFLOAT3 v3Min; XMFLOAT3 v3Max; XMFLOAT4X4 matRot; };
typedef SOBBData TOBBData;

struct SCylinderData { XMFLOAT3 v3Position; float fRadius; float fHeight; };
typedef SCylinderData TCylinderData;

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

class CBaseCollisionInstance
{
public:
    CBaseCollisionInstance() = default;
    virtual ~CBaseCollisionInstance();

    virtual void Render(D3D11_FILL_MODE fillMode = D3D11_FILL_SOLID) {}

    bool MovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const;
    bool CollisionDynamicSphere(const CDynamicSphereInstance& s) const;
    XMFLOAT3 GetCollisionMovementAdjust(const CDynamicSphereInstance& s) const;

    void Destroy();
    void ReleaseJoltBody() const;
    bool EnsureJoltBody(bool immediateQuery) const;
    bool HasJoltBody() const;
    const float3pos& GetJoltWorldCenter() const;
    float GetJoltRadius() const;

    static CBaseCollisionInstance* BuildCollisionInstance(const CStaticCollisionData* c_pCollisionData, const XMFLOAT4X4* pMat);

    void SetDebugInfo(DWORD type, const char* name);
    DWORD GetDebugType() const;
    const char* GetDebugName() const;

protected:
    virtual bool CreateJoltBodyInternal() = 0;
    virtual void OnDestroy() {}

    bool CreateStaticBox(const XMFLOAT3& center, const XMFLOAT3& halfSize, const XMFLOAT4& rotation);
    bool CreateStaticSphere(const XMFLOAT3& center, float radius);
    bool CreateStaticCylinder(const XMFLOAT3& center, float radius, float height, const XMFLOAT4& rotation);

    void SetJoltBounds(const XMFLOAT3& center, float radius);

protected:
    mutable PhysicsObjectComponent m_body;
    mutable bool m_joltCreated = false;
    float3pos m_joltWorldCenter = { 0.0f, 0.0f, 0.0f };
    float m_joltRadius = 0.0f;

    DWORD m_debugType = 0;
    char m_debugName[32] = {};
};

class CSphereCollisionInstance : public CBaseCollisionInstance
{
public:
    TSphereData& GetAttribute() { return m_attribute; }
    const TSphereData& GetAttribute() const { return m_attribute; }
protected:
    bool CreateJoltBodyInternal() override;
    TSphereData m_attribute{};
};

class CPlaneCollisionInstance : public CBaseCollisionInstance
{
public:
    TPlaneData& GetAttribute() { return m_attribute; }
    const TPlaneData& GetAttribute() const { return m_attribute; }
protected:
    bool CreateJoltBodyInternal() override;
    TPlaneData m_attribute{};
};

class CAABBCollisionInstance : public CBaseCollisionInstance
{
public:
    TAABBData& GetAttribute() { return m_attribute; }
    const TAABBData& GetAttribute() const { return m_attribute; }
protected:
    bool CreateJoltBodyInternal() override;
    TAABBData m_attribute{};
};

class COBBCollisionInstance : public CBaseCollisionInstance
{
public:
    TOBBData& GetAttribute() { return m_attribute; }
    const TOBBData& GetAttribute() const { return m_attribute; }
protected:
    bool CreateJoltBodyInternal() override;
    TOBBData m_attribute{};
};

class CCylinderCollisionInstance : public CBaseCollisionInstance
{
public:
    TCylinderData& GetAttribute() { return m_attribute; }
    const TCylinderData& GetAttribute() const { return m_attribute; }
protected:
    bool CreateJoltBodyInternal() override;
    TCylinderData m_attribute{};
};

typedef std::vector<CSphereCollisionInstance> CSphereCollisionInstanceVector;
typedef std::vector<CDynamicSphereInstance> CDynamicSphereInstanceVector;
typedef std::vector<CBaseCollisionInstance*> CCollisionInstanceVector;
