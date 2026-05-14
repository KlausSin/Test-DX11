#pragma once

#include <cstdint>
#include "CPhysicsManager.h"
#include "JoltPhysicsComponent.h"

enum class PhysicsCollisionType : uint8_t
{
    None,
    StaticBox,
    DynamicBox,
    KinematicBox,
    StaticSphere,
    DynamicSphere,
    StaticCapsule,
    DynamicCapsule,
    KinematicCapsule
};

class PhysicsObjectComponent
{
public:
    PhysicsObjectComponent() = default;
    ~PhysicsObjectComponent() { Destroy(); }

    bool CreateStaticBox(const Transform& transform, const float3pos& halfSize, void* owner = nullptr)
    {
        return CreateBox(transform, halfSize, 0.0f, owner, PhysicsCollisionType::StaticBox, JoltBodyType::Static);
    }

    bool CreateDynamicBox(const Transform& transform, const float3pos& halfSize, float mass, void* owner = nullptr)
    {
        return CreateBox(transform, halfSize, mass, owner, PhysicsCollisionType::DynamicBox, JoltBodyType::Dynamic);
    }

    bool CreateKinematicBox(const Transform& transform, const float3pos& halfSize, void* owner = nullptr)
    {
        return CreateBox(transform, halfSize, 1.0f, owner, PhysicsCollisionType::KinematicBox, JoltBodyType::Kinematic);
    }

    bool CreateStaticSphere(const Transform& transform, float radius, void* owner = nullptr)
    {
        return CreateSphere(transform, radius, 0.0f, owner, PhysicsCollisionType::StaticSphere, JoltBodyType::Static);
    }

    bool CreateDynamicSphere(const Transform& transform, float radius, float mass, void* owner = nullptr)
    {
        return CreateSphere(transform, radius, mass, owner, PhysicsCollisionType::DynamicSphere, JoltBodyType::Dynamic);
    }

    bool CreateStaticCapsule(const Transform& transform, float radius, float halfHeight, void* owner = nullptr)
    {
        return CreateCapsule(transform, radius, halfHeight, 0.0f, owner, PhysicsCollisionType::StaticCapsule, JoltBodyType::Static);
    }

    bool CreateDynamicCapsule(const Transform& transform, float radius, float halfHeight, float mass, void* owner = nullptr)
    {
        return CreateCapsule(transform, radius, halfHeight, mass, owner, PhysicsCollisionType::DynamicCapsule, JoltBodyType::Dynamic);
    }

    bool CreateKinematicCapsule(const Transform& transform, float radius, float halfHeight, void* owner = nullptr)
    {
        return CreateCapsule(transform, radius, halfHeight, 1.0f, owner, PhysicsCollisionType::KinematicCapsule, JoltBodyType::Kinematic);
    }

    void Destroy()
    {
        m_body.Destroy();
        m_type = PhysicsCollisionType::None;
        m_owner = nullptr;
    }

    void SyncFromPhysics(Transform& transform) const { m_body.SyncFromPhysics(transform); }
    void SyncToPhysics(const Transform& transform) { m_body.SyncToPhysics(transform); }
    void SetPosition(const float3pos& position) { m_body.SetPosition(position); }
    void SetRotation(const quatrot& rotation) { m_body.SetRotation(rotation); }
    void SetLinearVelocity(const float3pos& velocity) { m_body.SetLinearVelocity(velocity); }
    void SetAngularVelocity(const float3pos& velocity) { m_body.SetAngularVelocity(velocity); }
    void AddForce(const float3pos& force) { m_body.AddForce(force); }
    void AddImpulse(const float3pos& impulse) { m_body.AddImpulse(impulse); }
    void AddTorque(const float3pos& torque) { m_body.AddTorque(torque); }
    void SetGravityEnabled(bool enabled) { m_body.SetGravityEnabled(enabled); }
    void SetActive(bool active) { m_body.SetActive(active); }
    bool IsValid() const { return m_body.IsValid(); }
    void* GetOwner() const { return m_owner; }
    PhysicsCollisionType GetCollisionType() const { return m_type; }
    JoltPhysicsComponent& GetBody() { return m_body; }
    const JoltPhysicsComponent& GetBody() const { return m_body; }

private:
    bool CreateBox(const Transform& transform, const float3pos& halfSize, float mass, void* owner, PhysicsCollisionType collisionType, JoltBodyType bodyType)
    {
        Destroy();

        if (!CPhysicsManager::Instance().IsInitialized())
            return false;

        if (!m_body.CreateBox(CPhysicsManager::Instance().GetWorld(), transform, halfSize, mass, bodyType))
            return false;

        m_type = collisionType;
        m_owner = owner;
        m_body.SetUserData(owner);
        return true;
    }

    bool CreateSphere(const Transform& transform, float radius, float mass, void* owner, PhysicsCollisionType collisionType, JoltBodyType bodyType)
    {
        Destroy();

        if (!CPhysicsManager::Instance().IsInitialized())
            return false;

        if (!m_body.CreateSphere(CPhysicsManager::Instance().GetWorld(), transform, radius, mass, bodyType))
            return false;

        m_type = collisionType;
        m_owner = owner;
        m_body.SetUserData(owner);
        return true;
    }

    bool CreateCapsule(const Transform& transform, float radius, float halfHeight, float mass, void* owner, PhysicsCollisionType collisionType, JoltBodyType bodyType)
    {
        Destroy();

        if (!CPhysicsManager::Instance().IsInitialized())
            return false;

        if (!m_body.CreateCapsule(CPhysicsManager::Instance().GetWorld(), transform, radius, halfHeight, mass, bodyType))
            return false;

        m_type = collisionType;
        m_owner = owner;
        m_body.SetUserData(owner);
        return true;
    }

private:
    void* m_owner = nullptr;
    PhysicsCollisionType m_type = PhysicsCollisionType::None;
    JoltPhysicsComponent m_body;
};
