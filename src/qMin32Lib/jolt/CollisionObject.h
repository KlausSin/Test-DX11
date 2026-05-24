#pragma once

#include "CPhysicsManager.h"
#include "JoltPhysicsComponent.h"
#include "JoltShapeCache.h"
#include "CollisionTypes.h"

class CollisionObject
{
public:
    CollisionObject() = default;
    ~CollisionObject();

    CollisionObject(const CollisionObject&) = delete;
    CollisionObject& operator=(const CollisionObject&) = delete;

    bool CreateStaticBox(const float3pos& position, const float3pos& halfSize, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateStaticSphere(const float3pos& position, float radius, CollisionKind kind, void* owner = nullptr);
    bool CreateStaticCapsule(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateStaticCylinder(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateStaticShape(const float3pos& position, JPH::ShapeRefC shape, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateStaticMesh(const float3pos& position, const quatrot& rotation, const std::string& name, const JPH::VertexList& vertices, const JPH::IndexedTriangleList& triangles, CollisionKind kind, void* owner = nullptr);
    bool CreateStaticHeightField(const float3pos& position, const quatrot& rotation, const std::string& name, const std::vector<float>& samples, uint32_t size, float scaleX, float scaleY, float scaleZ, CollisionKind kind, void* owner = nullptr);

    bool CreateKinematicBox(const float3pos& position, const float3pos& halfSize, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateKinematicSphere(const float3pos& position, float radius, CollisionKind kind, void* owner = nullptr);
    bool CreateKinematicCapsule(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateKinematicCylinder(const float3pos& position, float radius, float halfHeight, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateKinematicShape(const float3pos& position, JPH::ShapeRefC shape, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);

    bool CreateDynamicBox(const float3pos& position, const float3pos& halfSize, float mass, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateDynamicSphere(const float3pos& position, float radius, float mass, CollisionKind kind, void* owner = nullptr);
    bool CreateDynamicCapsule(const float3pos& position, float radius, float halfHeight, float mass, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateDynamicCylinder(const float3pos& position, float radius, float halfHeight, float mass, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);
    bool CreateDynamicShape(const float3pos& position, JPH::ShapeRefC shape, float mass, const quatrot& rotation, CollisionKind kind, void* owner = nullptr);

    void Destroy();

    bool IsValid() const;
    void* GetOwner() const;
    CollisionKind GetKind() const;
    JoltBodyType GetBodyType() const;

    void SetPosition(const float3pos& position);
    void SetRotation(const quatrot& rotation);
    void SetTransform(const float3pos& position, const quatrot& rotation);
    void SetLinearVelocity(const float3pos& velocity);
    void SetAngularVelocity(const float3pos& velocity);
    float3pos GetLinearVelocity() const;
    void AddForce(const float3pos& force);
    void AddImpulse(const float3pos& impulse);
    void AddTorque(const float3pos& torque);
    void SetGravityEnabled(bool enabled);
    void SetActive(bool active);

    JoltPhysicsComponent& GetBody();
    const JoltPhysicsComponent& GetBody() const;

    static CollisionObject* FromUserData(void* userData);
    static const CollisionObject* FromUserData(const void* userData);

private:
    bool CreateBody(const Transform& transform, JPH::ShapeRefC shape, float mass, JoltBodyType type, CollisionKind kind, void* owner);

private:
    void* m_owner = nullptr;
    CollisionKind m_kind = CollisionKind::Unknown;
    JoltBodyType m_bodyType = JoltBodyType::Static;
    JoltPhysicsComponent m_body;
};

using MetinJoltCollisionObject = CollisionObject;
