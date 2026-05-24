#include "CollisionObject.h"

CollisionObject::~CollisionObject()
{
    Destroy();
}

bool CollisionObject::CreateStaticBox(const float3pos& position, const float3pos& halfSize, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetBoxShape(halfSize), 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateStaticSphere(const float3pos& position, float radius, CollisionKind kind, void* owner)
{
    return CreateBody({ position, { 0.0f, 0.0f, 0.0f, 1.0f } }, JoltShapeCache::Instance().GetSphereShape(radius), 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateStaticCapsule(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetCapsuleShape(radius, halfHeight), 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateStaticCylinder(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetCylinderShape(radius, halfHeight), 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateStaticShape(const float3pos& position, JPH::ShapeRefC shape, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, shape, 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateStaticMesh(const float3pos& position, const quatrot& rotation, const std::string& name, const JPH::VertexList& vertices, const JPH::IndexedTriangleList& triangles, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetMeshShape(name, vertices, triangles), 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateStaticHeightField(const float3pos& position, const quatrot& rotation, const std::string& name, const std::vector<float>& samples, uint32_t size, float scaleX, float scaleY, float scaleZ, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetHeightFieldShape(name, samples, size, scaleX, scaleY, scaleZ), 0.0f, JoltBodyType::Static, kind, owner);
}

bool CollisionObject::CreateKinematicBox(const float3pos& position, const float3pos& halfSize, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetBoxShape(halfSize), 1.0f, JoltBodyType::Kinematic, kind, owner);
}

bool CollisionObject::CreateKinematicSphere(const float3pos& position, float radius, CollisionKind kind, void* owner)
{
    return CreateBody({ position, { 0.0f, 0.0f, 0.0f, 1.0f } }, JoltShapeCache::Instance().GetSphereShape(radius), 1.0f, JoltBodyType::Kinematic, kind, owner);
}

bool CollisionObject::CreateKinematicCapsule(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetCapsuleShape(radius, halfHeight), 1.0f, JoltBodyType::Kinematic, kind, owner);
}

bool CollisionObject::CreateKinematicCylinder(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetCylinderShape(radius, halfHeight), 1.0f, JoltBodyType::Kinematic, kind, owner);
}

bool CollisionObject::CreateKinematicShape(const float3pos& position, JPH::ShapeRefC shape, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, shape, 1.0f, JoltBodyType::Kinematic, kind, owner);
}

bool CollisionObject::CreateDynamicBox(const float3pos& position, const float3pos& halfSize, float mass, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetBoxShape(halfSize), mass, JoltBodyType::Dynamic, kind, owner);
}

bool CollisionObject::CreateDynamicSphere(const float3pos& position, float radius, float mass, CollisionKind kind, void* owner)
{
    return CreateBody({ position, { 0.0f, 0.0f, 0.0f, 1.0f } }, JoltShapeCache::Instance().GetSphereShape(radius), mass, JoltBodyType::Dynamic, kind, owner);
}

bool CollisionObject::CreateDynamicCapsule(const float3pos& position, float radius, float halfHeight, float mass, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetCapsuleShape(radius, halfHeight), mass, JoltBodyType::Dynamic, kind, owner);
}

bool CollisionObject::CreateDynamicCylinder(const float3pos& position, float radius, float halfHeight, float mass, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, JoltShapeCache::Instance().GetCylinderShape(radius, halfHeight), mass, JoltBodyType::Dynamic, kind, owner);
}

bool CollisionObject::CreateDynamicShape(const float3pos& position, JPH::ShapeRefC shape, float mass, const quatrot& rotation, CollisionKind kind, void* owner)
{
    return CreateBody({ position, rotation }, shape, mass, JoltBodyType::Dynamic, kind, owner);
}

bool CollisionObject::CreateBody(const Transform& transform, JPH::ShapeRefC shape, float mass, JoltBodyType type, CollisionKind kind, void* owner)
{
    Destroy();

    if (!CPhysicsManager::Instance().IsInitialized() || !shape)
        return false;

    if (!m_body.CreateShape(CPhysicsManager::Instance().GetWorld(), transform, shape, mass, type))
        return false;

    m_owner = owner;
    m_kind = kind;
    m_bodyType = type;
    m_body.SetUserData(this);
    return true;
}

void CollisionObject::Destroy()
{
    m_body.Destroy();
    m_owner = nullptr;
    m_kind = CollisionKind::Unknown;
    m_bodyType = JoltBodyType::Static;
}

bool CollisionObject::IsValid() const { return m_body.IsValid(); }
void* CollisionObject::GetOwner() const { return m_owner; }
CollisionKind CollisionObject::GetKind() const { return m_kind; }
JoltBodyType CollisionObject::GetBodyType() const { return m_bodyType; }
void CollisionObject::SetPosition(const float3pos& position) { m_body.SetPosition(position); }
void CollisionObject::SetRotation(const quatrot& rotation) { m_body.SetRotation(rotation); }
void CollisionObject::SetTransform(const float3pos& position, const quatrot& rotation) { m_body.SyncToPhysics({ position, rotation }); }
void CollisionObject::SetLinearVelocity(const float3pos& velocity) { m_body.SetLinearVelocity(velocity); }
void CollisionObject::SetAngularVelocity(const float3pos& velocity) { m_body.SetAngularVelocity(velocity); }
float3pos CollisionObject::GetLinearVelocity() const { return m_body.GetLinearVelocity(); }
void CollisionObject::AddForce(const float3pos& force) { m_body.AddForce(force); }
void CollisionObject::AddImpulse(const float3pos& impulse) { m_body.AddImpulse(impulse); }
void CollisionObject::AddTorque(const float3pos& torque) { m_body.AddTorque(torque); }
void CollisionObject::SetGravityEnabled(bool enabled) { m_body.SetGravityEnabled(enabled); }
void CollisionObject::SetActive(bool active) { m_body.SetActive(active); }
JoltPhysicsComponent& CollisionObject::GetBody() { return m_body; }
const JoltPhysicsComponent& CollisionObject::GetBody() const { return m_body; }
CollisionObject* CollisionObject::FromUserData(void* userData) { return reinterpret_cast<CollisionObject*>(userData); }
const CollisionObject* CollisionObject::FromUserData(const void* userData) { return reinterpret_cast<const CollisionObject*>(userData); }
