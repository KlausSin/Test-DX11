#include <algorithm>
#include <thread>
#include <cstdarg>
#include <cstdio>

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/RayCast.h>
#include <Jolt/Physics/Collision/CastResult.h>
#include <Jolt/Physics/Collision/CollisionCollectorImpl.h>
#include <Jolt/Physics/Collision/CollideShape.h>
#include <Jolt/Physics/Collision/ShapeCast.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/Body/BodyLock.h>
#include <Jolt/Physics/Body/BodyLockInterface.h>
#include <Jolt/Core/Core.h>

#include "JoltPhysicsWorld.h"
#include "JoltConversion.h"
#include "JoltShapeCache.h"

#if __has_include(<EterBase/Debug.h>)
#include <EterBase/Debug.h>
#define JOLT_TRACE_ERROR(...) TraceError(__VA_ARGS__)
#else
#define JOLT_TRACE_ERROR(...)
#endif

JoltPhysicsWorld::~JoltPhysicsWorld()
{
    Shutdown();
}

static void JoltTrace(const char* fmt, ...)
{
    char buffer[2048];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);
    JOLT_TRACE_ERROR("[Jolt] %s", buffer);
}

static bool JoltAssert(const char* expression, const char* message, const char* file, uint32_t line)
{
    JOLT_TRACE_ERROR("[JoltAssert] %s | %s | %s:%u", expression ? expression : "", message ? message : "", file ? file : "", line);
    return false;
}

bool JoltPhysicsWorld::Initialize(uint32_t maxBodies, uint32_t numBodyMutexes, uint32_t maxBodyPairs, uint32_t maxContactConstraints)
{
    if (m_initialized)
        return true;

    JPH::RegisterDefaultAllocator();
    JPH::Trace = JoltTrace;
    JPH::AssertFailed = JoltAssert;

    m_factory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance = m_factory.get();
    JPH::RegisterTypes();

    m_tempAllocator = std::make_unique<JPH::TempAllocatorImpl>(64 * 1024 * 1024);

    const uint32_t hardwareThreads = std::thread::hardware_concurrency();
    const uint32_t threadCount = std::max(1u, hardwareThreads > 1 ? hardwareThreads - 1 : 1u);
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, int(threadCount));

    m_physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    m_physicsSystem->Init(maxBodies, numBodyMutexes, maxBodyPairs, maxContactConstraints, m_broadPhaseLayerInterface, m_objectVsBroadPhaseLayerFilter, m_objectLayerPairFilter);

    m_initialized = true;
    JOLT_TRACE_ERROR("Jolt initialized successfully");
    return true;
}

void JoltPhysicsWorld::Shutdown()
{
    if (!m_initialized)
        return;

    JoltShapeCache::Instance().Clear();
    m_physicsSystem.reset();
    m_jobSystem.reset();
    m_tempAllocator.reset();
    JPH::UnregisterTypes();
    JPH::Factory::sInstance = nullptr;
    m_factory.reset();
    m_initialized = false;
    JOLT_TRACE_ERROR("Jolt shutdown");
}

void JoltPhysicsWorld::Step(float deltaTime, int collisionSteps)
{
    if (!m_initialized || deltaTime <= 0.0f)
        return;

    if (deltaTime > 0.1f)
        deltaTime = 0.1f;

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

    const JPH::RVec3 point = ray.GetPointOnRay(result.mFraction);
    JPH::BodyLockRead lock(m_physicsSystem->GetBodyLockInterface(), result.mBodyID);
    if (!lock.Succeeded())
        return hit;

    const JPH::Body& body = lock.GetBody();
    hit.hit = true;
    hit.fraction = result.mFraction;
    hit.position = JoltConvert::FromRVec3(point);
    hit.normal = JoltConvert::FromVec3(body.GetWorldSpaceSurfaceNormal(result.mSubShapeID2, point));
    hit.bodyID = result.mBodyID.GetIndexAndSequenceNumber();
    hit.userData = reinterpret_cast<void*>(body.GetUserData());
    return hit;
}

JoltSphereQueryResult JoltPhysicsWorld::CollideSphere(const float3pos& position, float radius, const void* userDataFilter) const
{
    JoltSphereQueryResult result;

    if (!m_physicsSystem || radius <= 0.0f)
        return result;

    JPH::ShapeRefC shape = JoltShapeCache::Instance().GetSphereShape(radius);
    JPH::CollideShapeSettings settings;
    JPH::AllHitCollisionCollector<JPH::CollideShapeCollector> collector;

    const JPH::RMat44 transform = JPH::RMat44::sTranslation(JoltConvert::ToRVec3(position));
    m_physicsSystem->GetNarrowPhaseQuery().CollideShape(shape, JPH::Vec3::sReplicate(1.0f), transform, settings, JPH::RVec3::sZero(), collector);

    const JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();

    for (const JPH::CollideShapeResult& shapeHit : collector.mHits)
    {
        void* userData = reinterpret_cast<void*>(bodyInterface.GetUserData(shapeHit.mBodyID2));
        if (userDataFilter && userData != userDataFilter)
            continue;

        result.hit = true;
        result.fraction = 0.0f;
        result.bodyID = shapeHit.mBodyID2.GetIndexAndSequenceNumber();
        result.userData = userData;
        result.position = position;
        result.normal = JoltConvert::FromVec3(shapeHit.mPenetrationAxis.NormalizedOr(JPH::Vec3::sZero()));
        return result;
    }

    return result;
}

JoltSphereQueryResult JoltPhysicsWorld::CastSphere(const float3pos& from, const float3pos& to, float radius, const void* userDataFilter) const
{
    JoltSphereQueryResult result;

    if (!m_physicsSystem || radius <= 0.0f)
        return result;

    const JPH::Vec3 direction = JoltConvert::ToVec3({ to.x - from.x, to.y - from.y, to.z - from.z });
    if (direction.IsNearZero())
        return CollideSphere(to, radius, userDataFilter);

    JPH::ShapeRefC shape = JoltShapeCache::Instance().GetSphereShape(radius);
    JPH::RShapeCast shapeCast(shape, JPH::Vec3::sReplicate(1.0f), JPH::RMat44::sTranslation(JoltConvert::ToRVec3(from)), direction);
    JPH::ShapeCastSettings settings;
    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;

    m_physicsSystem->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

    const JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
    float bestFraction = 1.0f;
    const JPH::ShapeCastResult* bestHit = nullptr;
    void* bestUserData = nullptr;

    for (const JPH::ShapeCastResult& shapeHit : collector.mHits)
    {
        void* userData = reinterpret_cast<void*>(bodyInterface.GetUserData(shapeHit.mBodyID2));
        if (userDataFilter && userData != userDataFilter)
            continue;

        if (shapeHit.mFraction < bestFraction)
        {
            bestFraction = shapeHit.mFraction;
            bestHit = &shapeHit;
            bestUserData = userData;
        }
    }

    if (!bestHit)
        return result;

    result.hit = true;
    result.fraction = bestHit->mFraction;
    result.bodyID = bestHit->mBodyID2.GetIndexAndSequenceNumber();
    result.userData = bestUserData;
    result.position = { from.x + (to.x - from.x) * bestHit->mFraction, from.y + (to.y - from.y) * bestHit->mFraction, from.z + (to.z - from.z) * bestHit->mFraction };
    result.normal = JoltConvert::FromVec3(bestHit->mPenetrationAxis.NormalizedOr(JPH::Vec3::sZero()));
    return result;
}

JoltShapeCastHit JoltPhysicsWorld::CastCapsule(const float3pos& from, const float3pos& to, float radius, float halfHeight, const quatrot& rotation, const void* userDataFilter) const
{
    JoltShapeCastHit result;

    if (!m_physicsSystem || radius <= 0.0f || halfHeight <= 0.0f)
        return result;

    const JPH::Vec3 direction = JoltConvert::ToVec3({ to.x - from.x, to.y - from.y, to.z - from.z });
    if (direction.IsNearZero())
        return result;

    JPH::ShapeRefC shape = JoltShapeCache::Instance().GetCapsuleShape(radius, halfHeight);
    JPH::RMat44 transform = JPH::RMat44::sRotationTranslation(JoltConvert::ToQuat(rotation), JoltConvert::ToRVec3(from));
    JPH::RShapeCast shapeCast(shape, JPH::Vec3::sReplicate(1.0f), transform, direction);
    JPH::ShapeCastSettings settings;
    JPH::AllHitCollisionCollector<JPH::CastShapeCollector> collector;

    m_physicsSystem->GetNarrowPhaseQuery().CastShape(shapeCast, settings, JPH::RVec3::sZero(), collector);

    const JPH::BodyInterface& bodyInterface = m_physicsSystem->GetBodyInterface();
    float bestFraction = 1.0f;
    const JPH::ShapeCastResult* bestHit = nullptr;
    void* bestUserData = nullptr;

    for (const JPH::ShapeCastResult& shapeHit : collector.mHits)
    {
        void* userData = reinterpret_cast<void*>(bodyInterface.GetUserData(shapeHit.mBodyID2));
        if (userDataFilter && userData != userDataFilter)
            continue;

        if (shapeHit.mFraction < bestFraction)
        {
            bestFraction = shapeHit.mFraction;
            bestHit = &shapeHit;
            bestUserData = userData;
        }
    }

    if (!bestHit)
        return result;

    result.hit = true;
    result.fraction = bestHit->mFraction;
    result.bodyID = bestHit->mBodyID2.GetIndexAndSequenceNumber();
    result.userData = bestUserData;
    result.position = { from.x + (to.x - from.x) * bestHit->mFraction, from.y + (to.y - from.y) * bestHit->mFraction, from.z + (to.z - from.z) * bestHit->mFraction };
    result.normal = JoltConvert::FromVec3(bestHit->mPenetrationAxis.NormalizedOr(JPH::Vec3::sZero()));
    return result;
}

JPH::PhysicsSystem* JoltPhysicsWorld::GetSystem() { return m_physicsSystem.get(); }
const JPH::PhysicsSystem* JoltPhysicsWorld::GetSystem() const { return m_physicsSystem.get(); }
JPH::BodyInterface& JoltPhysicsWorld::GetBodyInterface() { return m_physicsSystem->GetBodyInterface(); }
const JPH::BodyLockInterface& JoltPhysicsWorld::GetBodyLockInterface() const { return m_physicsSystem->GetBodyLockInterface(); }
JPH::TempAllocator& JoltPhysicsWorld::GetTempAllocator() { return *m_tempAllocator; }
bool JoltPhysicsWorld::IsInitialized() const { return m_initialized; }
