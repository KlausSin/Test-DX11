#include <algorithm>
#include <thread>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>

#include "JoltPhysicsWorld.h"
#include "JoltConversion.h"
#include <Jolt/Core/Core.h>

JoltPhysicsWorld::~JoltPhysicsWorld()
{
    Shutdown();
}
static void DummyTrace(const char* inFMT, ...)
{
}

static bool DummyAssert(const char* inExpression, const char* inMessage, const char* inFile, uint32_t inLine)
{
    return false;
}

bool JoltPhysicsWorld::Initialize(uint32_t maxBodies, uint32_t numBodyMutexes, uint32_t maxBodyPairs, uint32_t maxContactConstraints)
{
    if (m_initialized)
        return true;

    JPH::RegisterDefaultAllocator();

    JPH::Trace = DummyTrace;
    JPH::AssertFailed = DummyAssert;

    m_factory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance = m_factory.get();
    JPH::RegisterTypes();

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(32 * 1024 * 1024);

    uint32_t hardwareThreads = std::thread::hardware_concurrency();
    uint32_t threadCount = std::max(1u, hardwareThreads > 1 ? hardwareThreads - 1 : 1u);
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, (int)threadCount);

    m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    m_physicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, m_broadPhaseLayerInterface, m_objectVsBroadPhaseLayerFilter, m_objectLayerPairFilter);

    m_initialized = true;
    return true;
}

void JoltPhysicsWorld::Shutdown()
{
    if (!m_initialized)
        return;

    m_physicsSystem.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();
    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;
    m_factory.reset();
    m_initialized = false;
}

void JoltPhysicsWorld::Step(float deltaTime, int collisionSteps)
{
    if (!m_initialized || deltaTime <= 0.0f)
        return;

    m_physicsSystem->Update(deltaTime, collisionSteps, m_tempAllocator.get(), m_jobSystem.get());
}

void JoltPhysicsWorld::OptimizeBroadPhase()
{
    if (m_physicsSystem)
        m_physicsSystem->OptimizeBroadPhase();
}

RaycastHit JoltPhysicsWorld::Raycast(const float3pos& origin, const float3pos& direction, float distance) const
{
    RaycastHit hit;

    if (!m_physicsSystem || distance <= 0.0f)
        return hit;

    JPH::RVec3 start = JoltConvert::ToRVec3(origin);
    JPH::Vec3 dir = JoltConvert::ToVec3(direction).NormalizedOr(JPH::Vec3::sZero()) * distance;

    if (dir.IsNearZero())
        return hit;

    JPH::RRayCast ray(start, dir);
    JPH::RayCastResult result;

    if (!m_physicsSystem->GetNarrowPhaseQuery().CastRay(ray, result))
        return hit;

    JPH::BodyID bodyID = result.mBodyID;
    JPH::RVec3 point = ray.GetPointOnRay(result.mFraction);

    JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), bodyID);
    if (!lock.Succeeded())
        return hit;

    const JPH::Body& body = lock.GetBody();

    hit.hit = true;
    hit.fraction = result.mFraction;
    hit.position = JoltConvert::FromRVec3(point);
    hit.normal = JoltConvert::FromVec3(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point));
    hit.bodyID = bodyID.GetIndexAndSequenceNumber();
    hit.userData = reinterpret_cast<void*>(body.GetUserData());

    return hit;
}

JPH::PhysicsSystem* JoltPhysicsWorld::GetSystem() { return m_physicsSystem.get(); }
const JPH::PhysicsSystem* JoltPhysicsWorld::GetSystem() const { return m_physicsSystem.get(); }
JPH::BodyInterface& JoltPhysicsWorld::GetBodyInterface() { return m_physicsSystem->GetBodyInterface(); }
const JPH::BodyLockInterface& JoltPhysicsWorld::GetBodyLockInterface() const { return m_physicsSystem->GetBodyLockInterface(); }
JPH::TempAllocator& JoltPhysicsWorld::GetTempAllocator() { return *m_tempAllocator; }
bool JoltPhysicsWorld::IsInitialized() const { return m_initialized; }
