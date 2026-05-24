#include "Stdafx.h"
#include "CollisionData.h"
#include "qMin32Lib/jolt/CPhysicsManager.h"
#include "qMin32Lib/jolt/JoltCollisionStreamer.h"
#include <algorithm>
#include <cmath>

using namespace DirectX;

static float3pos ToJolt(const XMFLOAT3& v) { return { v.x, -v.y, v.z }; }
static XMFLOAT3 FromJolt(const float3pos& v) { return XMFLOAT3(v.x, -v.y, v.z); }
static quatrot ToJoltQuat(const XMFLOAT4& q) { return { q.x, -q.y, q.z, q.w }; }
static XMFLOAT4 IdentityQuat() { return XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f); }
static XMFLOAT4 JoltYToZQuat() { return XMFLOAT4(0.70710678f, 0.0f, 0.0f, 0.70710678f); }
static XMFLOAT3 Add(const XMFLOAT3& a, const XMFLOAT3& b) { return XMFLOAT3(a.x + b.x, a.y + b.y, a.z + b.z); }
static XMFLOAT3 Sub(const XMFLOAT3& a, const XMFLOAT3& b) { return XMFLOAT3(a.x - b.x, a.y - b.y, a.z - b.z); }
static XMFLOAT3 Mul(const XMFLOAT3& a, float s) { return XMFLOAT3(a.x * s, a.y * s, a.z * s); }
static float Length2D(const XMFLOAT3& a, const XMFLOAT3& b)
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return sqrtf(dx * dx + dy * dy);
}

static XMFLOAT3 TransformPoint(const XMFLOAT3& point, const XMFLOAT4X4* matrix)
{
    XMFLOAT3 result;
    XMStoreFloat3(&result, XMVector3TransformCoord(XMLoadFloat3(&point), XMLoadFloat4x4(matrix)));
    return result;
}

static XMFLOAT4 ExtractRotationQuat(const XMFLOAT4X4* matrix)
{
    XMVECTOR scale;
    XMVECTOR rotation;
    XMVECTOR translation;
    if (!XMMatrixDecompose(&scale, &rotation, &translation, XMLoadFloat4x4(matrix)))
        return IdentityQuat();

    XMFLOAT4 result;
    XMStoreFloat4(&result, rotation);
    return result;
}

void DestroyCollisionInstanceSystem()
{
    JoltCollisionStreamer::Instance().Clear();
    CPhysicsManager::Instance().Shutdown();
}

CBaseCollisionInstance::~CBaseCollisionInstance()
{
    ReleaseJoltBody();
}

void CBaseCollisionInstance::Destroy()
{
    JoltCollisionStreamer::Instance().Unregister(this);
    ReleaseJoltBody();
    OnDestroy();
    delete this;
}

void CBaseCollisionInstance::ReleaseJoltBody() const
{
    if (m_joltCreated || m_body.IsValid())
        m_body.Destroy();

    m_joltCreated = false;
}

bool CBaseCollisionInstance::HasJoltBody() const
{
    return m_joltCreated && m_body.IsValid();
}

const float3pos& CBaseCollisionInstance::GetJoltWorldCenter() const
{
    return m_joltWorldCenter;
}

float CBaseCollisionInstance::GetJoltRadius() const
{
    return m_joltRadius;
}

void CBaseCollisionInstance::SetJoltBounds(const XMFLOAT3& center, float radius)
{
    m_joltWorldCenter = ToJolt(center);
    m_joltRadius = radius;
}

bool CBaseCollisionInstance::EnsureJoltBody(bool immediateQuery) const
{
    if (m_body.IsValid())
    {
        m_joltCreated = true;
        return true;
    }

    if (!CPhysicsManager::Instance().IsInitialized())
    {
        if (!CPhysicsManager::Instance().Initialize())
        {
            return false;
        }
    }

    CBaseCollisionInstance* self = const_cast<CBaseCollisionInstance*>(this);

    const bool ok = self->CreateJoltBodyInternal();
    self->m_joltCreated = ok && self->m_body.IsValid();

    return self->m_joltCreated;
}

bool CBaseCollisionInstance::MovementCollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
    if (!EnsureJoltBody(true) || !CPhysicsManager::Instance().IsInitialized())
        return false;

    const JoltSphereQueryResult hit = CPhysicsManager::Instance().CastSphere(ToJolt(s.v3LastPosition), ToJolt(s.v3Position), s.fRadius, this);
    return hit.hit;
}

bool CBaseCollisionInstance::CollisionDynamicSphere(const CDynamicSphereInstance& s) const
{
    if (!EnsureJoltBody(true) || !CPhysicsManager::Instance().IsInitialized())
        return false;

    const JoltSphereQueryResult hit = CPhysicsManager::Instance().CollideSphere(ToJolt(s.v3Position), s.fRadius, this);
    return hit.hit;
}

void CBaseCollisionInstance::SetDebugInfo(DWORD type, const char* name)
{
    m_debugType = type;
    strncpy(m_debugName, name ? name : "", sizeof(m_debugName) - 1);
    m_debugName[sizeof(m_debugName) - 1] = 0;
}

DWORD CBaseCollisionInstance::GetDebugType() const
{
    return m_debugType;
}

const char* CBaseCollisionInstance::GetDebugName() const
{
    return m_debugName;
}

XMFLOAT3 CBaseCollisionInstance::GetCollisionMovementAdjust(const CDynamicSphereInstance& s) const
{
    if (!EnsureJoltBody(true) || !CPhysicsManager::Instance().IsInitialized())
        return XMFLOAT3(0.0f, 0.0f, 0.0f);

    const JoltSphereQueryResult hit = CPhysicsManager::Instance().CastSphere(ToJolt(s.v3LastPosition), ToJolt(s.v3Position), s.fRadius, this);
    if (!hit.hit)
        return XMFLOAT3(0.0f, 0.0f, 0.0f);

    XMFLOAT3 n = FromJolt(hit.normal);
    return XMFLOAT3(n.x * s.fRadius, n.y * s.fRadius, n.z * s.fRadius);
}

bool CBaseCollisionInstance::CreateStaticBox(const XMFLOAT3& center, const XMFLOAT3& halfSize, const XMFLOAT4& rotation)
{
    Transform transform;
    transform.position = ToJolt(center);
    transform.rotation = ToJoltQuat(rotation);
    return m_body.CreateStaticBox(transform, { halfSize.x, halfSize.y, halfSize.z }, this);
}

bool CBaseCollisionInstance::CreateStaticSphere(const XMFLOAT3& center, float radius)
{
    Transform transform;
    transform.position = ToJolt(center);
    transform.rotation = { 0.0f, 0.0f, 0.0f, 1.0f };

    bool ok = m_body.CreateStaticSphere(transform, radius, this);

    return ok;
}

bool CBaseCollisionInstance::CreateStaticCylinder(const XMFLOAT3& center, float radius, float halfHeight, const XMFLOAT4& rotation)
{
    Transform transform;
    transform.position = ToJolt(center);
    transform.rotation = ToJoltQuat(rotation);

    bool ok = m_body.CreateStaticCylinder(transform, radius, halfHeight, this);

    return ok;
}

bool CSphereCollisionInstance::CreateJoltBodyInternal()
{
    return CreateStaticSphere(m_attribute.v3Position, m_attribute.fRadius);
}

bool CPlaneCollisionInstance::CreateJoltBodyInternal()
{
    XMFLOAT3 minp = m_attribute.v3QuadPosition[0];
    XMFLOAT3 maxp = m_attribute.v3QuadPosition[0];
    for (uint32_t i = 1; i < 4; ++i)
    {
        const XMFLOAT3& p = m_attribute.v3QuadPosition[i];
        minp.x = std::min(minp.x, p.x); minp.y = std::min(minp.y, p.y); minp.z = std::min(minp.z, p.z);
        maxp.x = std::max(maxp.x, p.x); maxp.y = std::max(maxp.y, p.y); maxp.z = std::max(maxp.z, p.z);
    }

    const float thickness = JoltCollisionStreamer::Instance().GetConfig().defaultPlaneThickness;
    XMFLOAT3 center = Mul(Add(minp, maxp), 0.5f);
    XMFLOAT3 halfSize = Mul(Sub(maxp, minp), 0.5f);
    halfSize.x = std::max(halfSize.x, thickness);
    halfSize.y = std::max(halfSize.y, thickness);
    halfSize.z = std::max(halfSize.z, thickness);
    return CreateStaticBox(center, halfSize, IdentityQuat());
}

bool CAABBCollisionInstance::CreateJoltBodyInternal()
{
    const XMFLOAT3 center = Mul(Add(m_attribute.v3Min, m_attribute.v3Max), 0.5f);
    const XMFLOAT3 halfSize = Mul(Sub(m_attribute.v3Max, m_attribute.v3Min), 0.5f);
    return CreateStaticBox(center, halfSize, IdentityQuat());
}

bool COBBCollisionInstance::CreateJoltBodyInternal()
{
    const XMFLOAT3 center = Mul(Add(m_attribute.v3Min, m_attribute.v3Max), 0.5f);
    const XMFLOAT3 halfSize = Mul(Sub(m_attribute.v3Max, m_attribute.v3Min), 0.5f);
    return CreateStaticBox(center, halfSize, ExtractRotationQuat(&m_attribute.matRot));
}

bool CCylinderCollisionInstance::CreateJoltBodyInternal()
{
    float radius = m_attribute.fRadius;
    float halfHeight = m_attribute.fHeight * 0.5f;

    if (radius <= 1.0f || halfHeight <= 1.0f)
    {
        return false;
    }

    return CreateStaticCylinder(
        m_attribute.v3Position,
        radius,
        halfHeight,
        JoltYToZQuat()
    );
}

CBaseCollisionInstance* CBaseCollisionInstance::BuildCollisionInstance(const CStaticCollisionData* c_pCollisionData, const XMFLOAT4X4* pMat)
{
    if (!c_pCollisionData || !pMat)
        return nullptr;

    const XMMATRIX matWorld = XMLoadFloat4x4(pMat);
    CBaseCollisionInstance* base = nullptr;

    switch (c_pCollisionData->dwType)
    {
        case COLLISION_TYPE_PLANE:
        {
            CPlaneCollisionInstance* instance = new CPlaneCollisionInstance();
            TPlaneData& data = instance->GetAttribute();
            const float halfWidth = c_pCollisionData->fDimensions[0] * 0.5f;
            const float halfLength = c_pCollisionData->fDimensions[1] * 0.5f;
            XMFLOAT3 local[4] =
            {
                XMFLOAT3(-halfWidth, -halfLength, 0.0f),
                XMFLOAT3(+halfWidth, -halfLength, 0.0f),
                XMFLOAT3(-halfWidth, +halfLength, 0.0f),
                XMFLOAT3(+halfWidth, +halfLength, 0.0f),
            };

            XMMATRIX matRotation = XMMatrixRotationQuaternion(XMLoadFloat4(&c_pCollisionData->quatRotation));
            XMMATRIX matTranslation = XMMatrixTranslation(c_pCollisionData->v3Position.x, c_pCollisionData->v3Position.y, c_pCollisionData->v3Position.z);
            XMMATRIX matTransform = matRotation * matTranslation * matWorld;

            XMStoreFloat3(&data.v3Position, XMVector3TransformCoord(XMLoadFloat3(&c_pCollisionData->v3Position), matWorld));
            for (uint32_t i = 0; i < 4; ++i)
                XMStoreFloat3(&data.v3QuadPosition[i], XMVector3TransformCoord(XMLoadFloat3(&local[i]), matTransform));

            XMVECTOR line0 = XMVector3Normalize(XMLoadFloat3(&data.v3QuadPosition[1]) - XMLoadFloat3(&data.v3QuadPosition[0]));
            XMVECTOR line1 = XMVector3Normalize(XMLoadFloat3(&data.v3QuadPosition[2]) - XMLoadFloat3(&data.v3QuadPosition[0]));
            XMVECTOR normal = XMVector3Normalize(XMVector3Cross(line0, line1));
            XMStoreFloat3(&data.v3Normal, normal);

            XMFLOAT3 center = data.v3Position;
            float radius = std::max(c_pCollisionData->fDimensions[0], c_pCollisionData->fDimensions[1]) * 0.75f;
            instance->SetJoltBounds(center, radius);
            base = instance;
            break;
        }

        case COLLISION_TYPE_BOX:
        {
            COBBCollisionInstance* instance = new COBBCollisionInstance();

            XMFLOAT3 center = TransformPoint(c_pCollisionData->v3Position, pMat);

            XMFLOAT3 half(
                c_pCollisionData->fDimensions[0],
                c_pCollisionData->fDimensions[1],
                c_pCollisionData->fDimensions[2]
            );

            TOBBData& data = instance->GetAttribute();

            data.v3Min = Sub(center, half);
            data.v3Max = Add(center, half);

            XMMATRIX localRot = XMMatrixRotationQuaternion(
                XMLoadFloat4(&c_pCollisionData->quatRotation)
            );

            XMMATRIX world = XMLoadFloat4x4(pMat);

            XMStoreFloat4x4(&data.matRot, localRot * world);

            instance->SetJoltBounds(center,
                std::max(std::max(half.x, half.y), half.z));

            base = instance;
            break;
        }
        case COLLISION_TYPE_AABB:
        {
            CAABBCollisionInstance* instance = new CAABBCollisionInstance();
            XMFLOAT3 center = TransformPoint(c_pCollisionData->v3Position, pMat);
            XMFLOAT3 half(c_pCollisionData->fDimensions[0], c_pCollisionData->fDimensions[1], c_pCollisionData->fDimensions[2]);
            TAABBData& data = instance->GetAttribute();
            data.v3Min = Sub(center, half);
            data.v3Max = Add(center, half);
            instance->SetJoltBounds(center, std::max(std::max(half.x, half.y), half.z));
            base = instance;
            break;
        }

        case COLLISION_TYPE_OBB:
        {
            COBBCollisionInstance* instance = new COBBCollisionInstance();
            XMFLOAT3 center = TransformPoint(c_pCollisionData->v3Position, pMat);
            XMFLOAT3 half(c_pCollisionData->fDimensions[0], c_pCollisionData->fDimensions[1], c_pCollisionData->fDimensions[2]);
            TOBBData& data = instance->GetAttribute();
            data.v3Min = Sub(center, half);
            data.v3Max = Add(center, half);
            data.matRot = *pMat;
            instance->SetJoltBounds(center, std::max(std::max(half.x, half.y), half.z));
            base = instance;
            break;
        }

        case COLLISION_TYPE_SPHERE:
        {
            CSphereCollisionInstance* instance = new CSphereCollisionInstance();
            TSphereData& data = instance->GetAttribute();
            data.v3Position = TransformPoint(c_pCollisionData->v3Position, pMat);
            data.fRadius = c_pCollisionData->fDimensions[0];
            instance->SetJoltBounds(data.v3Position, data.fRadius);
            base = instance;
            break;
        }

        case COLLISION_TYPE_CYLINDER:
        {
            CCylinderCollisionInstance* instance = new CCylinderCollisionInstance();
            TCylinderData& data = instance->GetAttribute();
            data.v3Position = TransformPoint(c_pCollisionData->v3Position, pMat);
            data.fRadius = c_pCollisionData->fDimensions[0];
            data.fHeight = c_pCollisionData->fDimensions[1];
            instance->SetJoltBounds(data.v3Position, std::max(data.fRadius, data.fHeight * 0.5f));
            base = instance;
            break;
        }
    }

    if (base)
    {
        JoltCollisionStreamer::Instance().Register(base);
    }

    return base;
}
