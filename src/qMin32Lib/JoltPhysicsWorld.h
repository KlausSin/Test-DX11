#pragma once

#include <memory>
#include <cstdint>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>

#include "JoltTypes.h"
#include "JoltPhysicsLayers.h"

class JoltPhysicsWorld
{
public:
    JoltPhysicsWorld() = default;
    ~JoltPhysicsWorld();

    bool Initialize(uint32_t maxBodies = 65536, uint32_t numBodyMutexes = 0, uint32_t maxBodyPairs = 65536, uint32_t maxContactConstraints = 10240);
    void Shutdown();
    void Step(float deltaTime, int collisionSteps = 1);
    void OptimizeBroadPhase();

    RaycastHit Raycast(const float3pos& origin, const float3pos& direction, float distance) const;

    JPH::PhysicsSystem* GetSystem();
    const JPH::PhysicsSystem* GetSystem() const;
    JPH::BodyInterface& GetBodyInterface();
    const JPH::BodyLockInterface& GetBodyLockInterface() const;
    JPH::TempAllocator& GetTempAllocator();
    bool IsInitialized() const;

private:
    bool m_initialized = false;
    std::unique_ptr<JPH::Factory> m_factory;
    std::unique_ptr<JPH::TempAllocatorImpl> m_tempAllocator;
    std::unique_ptr<JPH::JobSystemThreadPool> m_jobSystem;
    std::unique_ptr<JPH::PhysicsSystem> m_physicsSystem;
    JoltBPLayerInterface m_broadPhaseLayerInterface;
    JoltObjectVsBroadPhaseLayerFilter m_objectVsBroadPhaseLayerFilter;
    JoltObjectLayerPairFilter m_objectLayerPairFilter;
};
