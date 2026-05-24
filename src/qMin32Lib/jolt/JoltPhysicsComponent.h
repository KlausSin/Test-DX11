#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include "JoltTypes.h"

class JoltPhysicsWorld;

class JoltPhysicsComponent
{
public:
    JoltPhysicsComponent() = default;
    ~JoltPhysicsComponent();

    bool CreateBox(JoltPhysicsWorld& world, const Transform& transform, const float3pos& halfSize, float mass, JoltBodyType type);
    bool CreateSphere(JoltPhysicsWorld& world, const Transform& transform, float radius, float mass, JoltBodyType type);
    bool CreateCapsule(JoltPhysicsWorld& world, const Transform& transform, float radius, float halfHeight, float mass, JoltBodyType type);
    bool CreateCylinder(JoltPhysicsWorld& world, const Transform& transform, float radius, float halfHeight, float mass, JoltBodyType type);
    bool CreateShape(JoltPhysicsWorld& world, const Transform& transform, JPH::ShapeRefC shape, float mass, JoltBodyType type);

    void Destroy();
    void SyncFromPhysics(Transform& transform) const;
    void SyncToPhysics(const Transform& transform);

    void SetPosition(const float3pos& position);
    void SetRotation(const quatrot& rotation);
    void SetLinearVelocity(const float3pos& velocity);
    void SetAngularVelocity(const float3pos& velocity);
    float3pos GetLinearVelocity() const;
    void AddForce(const float3pos& force);
    void AddImpulse(const float3pos& impulse);
    void AddTorque(const float3pos& torque);
    void SetGravityEnabled(bool enabled);
    void SetActive(bool active);
    void SetUserData(void* userData);
    void* GetUserData() const;

    bool IsValid() const;
    JPH::BodyID GetBodyID() const;
    JoltBodyType GetType() const;

private:
    bool CreateBody(JoltPhysicsWorld& world, const JPH::ShapeSettings& shapeSettings, const Transform& transform, float mass, JoltBodyType type);

private:
    JoltPhysicsWorld* m_world = nullptr;
    JPH::BodyID m_bodyID ={};
    bool m_valid = false;
    JoltBodyType m_type = JoltBodyType::Static;
};
