#include <Jolt/Jolt.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyInterface.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/CylinderShape.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>

#include "JoltPhysicsComponent.h"
#include "JoltPhysicsWorld.h"
#include "JoltPhysicsLayers.h"
#include "JoltConversion.h"
#include "JoltShapeCache.h"
#include "CPhysicsManager.h"

namespace
{
    JPH::PhysicsSystem* GetValidPhysicsSystem()
    {
        if (!CPhysicsManager::Instance().IsInitialized())
            return nullptr;

        return CPhysicsManager::Instance().GetWorld().GetSystem();
    }

    JPH::BodyInterface* GetValidBodyInterface()
    {
        JPH::PhysicsSystem* system = GetValidPhysicsSystem();
        if (!system)
            return nullptr;

        return &system->GetBodyInterface();
    }
}

JoltPhysicsComponent::~JoltPhysicsComponent()
{
    Destroy();
}

bool JoltPhysicsComponent::CreateBox(JoltPhysicsWorld& world, const Transform& transform, const float3pos& halfSize, float mass, JoltBodyType type)
{
    return CreateShape(world, transform, JoltShapeCache::Instance().GetBoxShape(halfSize), mass, type);
}

bool JoltPhysicsComponent::CreateSphere(JoltPhysicsWorld& world, const Transform& transform, float radius, float mass, JoltBodyType type)
{
    return CreateShape(world, transform, JoltShapeCache::Instance().GetSphereShape(radius), mass, type);
}

bool JoltPhysicsComponent::CreateCapsule(JoltPhysicsWorld& world, const Transform& transform, float radius, float halfHeight, float mass, JoltBodyType type)
{
    return CreateShape(world, transform, JoltShapeCache::Instance().GetCapsuleShape(radius, halfHeight), mass, type);
}

bool JoltPhysicsComponent::CreateCylinder(JoltPhysicsWorld& world, const Transform& transform, float radius, float halfHeight, float mass, JoltBodyType type)
{
    return CreateShape(world, transform, JoltShapeCache::Instance().GetCylinderShape(radius, halfHeight), mass, type);
}

bool JoltPhysicsComponent::CreateShape(JoltPhysicsWorld& world, const Transform& transform, JPH::ShapeRefC shape, float mass, JoltBodyType type)
{
    Destroy();

    if (!world.IsInitialized() || !shape)
        return false;

    JPH::PhysicsSystem* system = world.GetSystem();
    if (!system)
        return false;

    JPH::EMotionType motionType = JPH::EMotionType::Static;
    JPH::ObjectLayer objectLayer = JoltObjectLayers::NON_MOVING;

    if (type == JoltBodyType::Dynamic || type == JoltBodyType::Kinematic)
    {
        motionType = type == JoltBodyType::Kinematic ? JPH::EMotionType::Kinematic : JPH::EMotionType::Dynamic;
        objectLayer = JoltObjectLayers::MOVING;
    }

    JPH::BodyCreationSettings bodySettings(shape, JoltConvert::ToRVec3(transform.position), JoltConvert::ToQuat(transform.rotation), motionType, objectLayer);
    bodySettings.mOverrideMassProperties = type == JoltBodyType::Dynamic ? JPH::EOverrideMassProperties::CalculateInertia : JPH::EOverrideMassProperties::MassAndInertiaProvided;
    bodySettings.mMassPropertiesOverride.mMass = mass > 0.0f ? mass : 1.0f;

    JPH::BodyInterface& bodyInterface = system->GetBodyInterface();
    JPH::Body* body = bodyInterface.CreateBody(bodySettings);
    if (!body)
        return false;

    m_bodyID = body->GetID();
    m_world = &world;
    m_type = type;
    m_valid = true;

    bodyInterface.AddBody(m_bodyID, type == JoltBodyType::Static ? JPH::EActivation::DontActivate : JPH::EActivation::Activate);
    return true;
}

bool JoltPhysicsComponent::CreateBody(JoltPhysicsWorld& world, const JPH::ShapeSettings& shapeSettings, const Transform& transform, float mass, JoltBodyType type)
{
    JPH::ShapeSettings::ShapeResult shapeResult = shapeSettings.Create();
    if (shapeResult.HasError())
        return false;

    return CreateShape(world, transform, shapeResult.Get(), mass, type);
}

void JoltPhysicsComponent::Destroy()
{
    if (!m_valid)
    {
        m_world = nullptr;
        return;
    }

    JPH::BodyID bodyID = m_bodyID;

    m_valid = false;
    m_world = nullptr;
    m_bodyID = {};
    m_type = JoltBodyType::Static;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (!bodyInterface)
        return;

    if (bodyInterface->IsAdded(bodyID))
        bodyInterface->RemoveBody(bodyID);

    bodyInterface->DestroyBody(bodyID);
}

void JoltPhysicsComponent::SyncFromPhysics(Transform& transform) const
{
    if (!m_valid)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (!bodyInterface)
        return;

    transform.position = JoltConvert::FromRVec3(bodyInterface->GetCenterOfMassPosition(m_bodyID));
    transform.rotation = JoltConvert::FromQuat(bodyInterface->GetRotation(m_bodyID));
}

void JoltPhysicsComponent::SyncToPhysics(const Transform& transform)
{
    if (!m_valid)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (!bodyInterface)
        return;

    bodyInterface->SetPositionAndRotation(m_bodyID, JoltConvert::ToRVec3(transform.position), JoltConvert::ToQuat(transform.rotation), JPH::EActivation::Activate);
}

void JoltPhysicsComponent::SetPosition(const float3pos& position)
{
    if (!m_valid)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->SetPosition(m_bodyID, JoltConvert::ToRVec3(position), JPH::EActivation::Activate);
}

void JoltPhysicsComponent::SetRotation(const quatrot& rotation)
{
    if (!m_valid)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->SetRotation(m_bodyID, JoltConvert::ToQuat(rotation), JPH::EActivation::Activate);
}

void JoltPhysicsComponent::SetLinearVelocity(const float3pos& velocity)
{
    if (!m_valid || m_type == JoltBodyType::Static)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->SetLinearVelocity(m_bodyID, JoltConvert::ToVec3(velocity));
}

void JoltPhysicsComponent::SetAngularVelocity(const float3pos& velocity)
{
    if (!m_valid || m_type == JoltBodyType::Static)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->SetAngularVelocity(m_bodyID, JoltConvert::ToVec3(velocity));
}

float3pos JoltPhysicsComponent::GetLinearVelocity() const
{
    if (!m_valid || m_type == JoltBodyType::Static)
        return { 0.0f, 0.0f, 0.0f };

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (!bodyInterface)
        return { 0.0f, 0.0f, 0.0f };

    return JoltConvert::FromVec3(bodyInterface->GetLinearVelocity(m_bodyID));
}

void JoltPhysicsComponent::AddForce(const float3pos& force)
{
    if (!m_valid || m_type != JoltBodyType::Dynamic)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->AddForce(m_bodyID, JoltConvert::ToVec3(force));
}

void JoltPhysicsComponent::AddImpulse(const float3pos& impulse)
{
    if (!m_valid || m_type != JoltBodyType::Dynamic)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->AddImpulse(m_bodyID, JoltConvert::ToVec3(impulse));
}

void JoltPhysicsComponent::AddTorque(const float3pos& torque)
{
    if (!m_valid || m_type != JoltBodyType::Dynamic)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->AddTorque(m_bodyID, JoltConvert::ToVec3(torque));
}

void JoltPhysicsComponent::SetGravityEnabled(bool enabled)
{
    if (!m_valid || m_type == JoltBodyType::Static)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->SetGravityFactor(m_bodyID, enabled ? 1.0f : 0.0f);
}

void JoltPhysicsComponent::SetActive(bool active)
{
    if (!m_valid)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (!bodyInterface)
        return;

    if (active)
        bodyInterface->ActivateBody(m_bodyID);
    else
        bodyInterface->DeactivateBody(m_bodyID);
}

void JoltPhysicsComponent::SetUserData(void* userData)
{
    if (!m_valid)
        return;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (bodyInterface)
        bodyInterface->SetUserData(m_bodyID, reinterpret_cast<JPH::uint64>(userData));
}

void* JoltPhysicsComponent::GetUserData() const
{
    if (!m_valid)
        return nullptr;

    JPH::BodyInterface* bodyInterface = GetValidBodyInterface();
    if (!bodyInterface)
        return nullptr;

    return reinterpret_cast<void*>(bodyInterface->GetUserData(m_bodyID));
}

bool JoltPhysicsComponent::IsValid() const { return m_valid; }
JPH::BodyID JoltPhysicsComponent::GetBodyID() const { return m_bodyID; }
JoltBodyType JoltPhysicsComponent::GetType() const { return m_type; }
