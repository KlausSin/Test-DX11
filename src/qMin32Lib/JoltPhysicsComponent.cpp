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

    JPH::BodyInterface& bodyInterface = world.GetBodyInterface();
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

    if (!m_world)
    {
        m_valid = false;
        return;
    }

    if (reinterpret_cast<uintptr_t>(m_world) == UINTPTR_MAX)
    {
        m_valid = false;
        m_world = nullptr;
        return;
    }

    if (!m_world->IsInitialized())
    {
        m_valid = false;
        m_world = nullptr;
        return;
    }

    JPH::BodyInterface& bodyInterface = m_world->GetBodyInterface();

    if (bodyInterface.IsAdded(m_bodyID))
        bodyInterface.RemoveBody(m_bodyID);

    bodyInterface.DestroyBody(m_bodyID);

    m_valid = false;
    m_world = nullptr;
}

void JoltPhysicsComponent::SyncFromPhysics(Transform& transform) const
{
    if (!m_valid || !m_world)
        return;

    const JPH::BodyInterface& bodyInterface = m_world->GetSystem()->GetBodyInterface();
    transform.position = JoltConvert::FromRVec3(bodyInterface.GetCenterOfMassPosition(m_bodyID));
    transform.rotation = JoltConvert::FromQuat(bodyInterface.GetRotation(m_bodyID));
}

void JoltPhysicsComponent::SyncToPhysics(const Transform& transform)
{
    if (!m_valid || !m_world)
        return;

    JPH::BodyInterface& bodyInterface = m_world->GetBodyInterface();
    bodyInterface.SetPositionAndRotation(m_bodyID, JoltConvert::ToRVec3(transform.position), JoltConvert::ToQuat(transform.rotation), JPH::EActivation::Activate);
}

void JoltPhysicsComponent::SetPosition(const float3pos& position)
{
    if (m_valid && m_world)
        m_world->GetBodyInterface().SetPosition(m_bodyID, JoltConvert::ToRVec3(position), JPH::EActivation::Activate);
}

void JoltPhysicsComponent::SetRotation(const quatrot& rotation)
{
    if (m_valid && m_world)
        m_world->GetBodyInterface().SetRotation(m_bodyID, JoltConvert::ToQuat(rotation), JPH::EActivation::Activate);
}

void JoltPhysicsComponent::SetLinearVelocity(const float3pos& velocity)
{
    if (m_valid && m_world && m_type != JoltBodyType::Static)
        m_world->GetBodyInterface().SetLinearVelocity(m_bodyID, JoltConvert::ToVec3(velocity));
}

void JoltPhysicsComponent::SetAngularVelocity(const float3pos& velocity)
{
    if (m_valid && m_world && m_type != JoltBodyType::Static)
        m_world->GetBodyInterface().SetAngularVelocity(m_bodyID, JoltConvert::ToVec3(velocity));
}

float3pos JoltPhysicsComponent::GetLinearVelocity() const
{
    if (!m_valid || !m_world || m_type == JoltBodyType::Static)
        return { 0.0f, 0.0f, 0.0f };

    return JoltConvert::FromVec3(m_world->GetSystem()->GetBodyInterface().GetLinearVelocity(m_bodyID));
}

void JoltPhysicsComponent::AddForce(const float3pos& force)
{
    if (m_valid && m_world && m_type == JoltBodyType::Dynamic)
        m_world->GetBodyInterface().AddForce(m_bodyID, JoltConvert::ToVec3(force));
}

void JoltPhysicsComponent::AddImpulse(const float3pos& impulse)
{
    if (m_valid && m_world && m_type == JoltBodyType::Dynamic)
        m_world->GetBodyInterface().AddImpulse(m_bodyID, JoltConvert::ToVec3(impulse));
}

void JoltPhysicsComponent::AddTorque(const float3pos& torque)
{
    if (m_valid && m_world && m_type == JoltBodyType::Dynamic)
        m_world->GetBodyInterface().AddTorque(m_bodyID, JoltConvert::ToVec3(torque));
}

void JoltPhysicsComponent::SetGravityEnabled(bool enabled)
{
    if (m_valid && m_world && m_type != JoltBodyType::Static)
        m_world->GetBodyInterface().SetGravityFactor(m_bodyID, enabled ? 1.0f : 0.0f);
}

void JoltPhysicsComponent::SetActive(bool active)
{
    if (!m_valid || !m_world)
        return;

    if (active)
        m_world->GetBodyInterface().ActivateBody(m_bodyID);
    else
        m_world->GetBodyInterface().DeactivateBody(m_bodyID);
}

void JoltPhysicsComponent::SetUserData(void* userData)
{
    if (m_valid && m_world)
        m_world->GetBodyInterface().SetUserData(m_bodyID, reinterpret_cast<JPH::uint64>(userData));
}

void* JoltPhysicsComponent::GetUserData() const
{
    if (!m_valid || !m_world)
        return nullptr;

    return reinterpret_cast<void*>(m_world->GetSystem()->GetBodyInterface().GetUserData(m_bodyID));
}

bool JoltPhysicsComponent::IsValid() const { return m_valid; }
JPH::BodyID JoltPhysicsComponent::GetBodyID() const { return m_bodyID; }
JoltBodyType JoltPhysicsComponent::GetType() const { return m_type; }
