#pragma once

#include "JoltPhysicsWorld.h"
#include "JoltCollisionStreamer.h"

class CPhysicsManager
{
public:
    static CPhysicsManager& Instance()
    {
        static CPhysicsManager instance;
        return instance;
    }

    bool Initialize()
    {
        if (m_initialized)
            return true;

        if (!m_world.Initialize())
            return false;

        m_initialized = true;
        return true;
    }

    void Shutdown()
    {
        if (!m_initialized)
            return;

        m_world.Shutdown();
        m_initialized = false;
    }

    void Update(float dt)
    {
        if (!m_initialized)
            return;

        JoltCollisionStreamer::Instance().Update(dt);
        m_world.Step(dt);
    }

    void SetStreamCenter(const float3pos& center)
    {
        JoltCollisionStreamer::Instance().SetCenter(center);
    }

    void SetStreamConfig(const JoltPhysicsConfig& config)
    {
        JoltCollisionStreamer::Instance().SetConfig(config);
    }

    void OptimizeBroadPhase()
    {
        if (m_initialized)
            m_world.OptimizeBroadPhase();
    }

    RaycastHit Raycast(const float3pos& origin, const float3pos& direction, float distance) const
    {
        return m_world.Raycast(origin, direction, distance);
    }

    JoltSphereQueryResult CollideSphere(const float3pos& position, float radius, const void* userDataFilter = nullptr) const
    {
        return m_world.CollideSphere(position, radius, userDataFilter);
    }

    JoltSphereQueryResult CastSphere(const float3pos& from, const float3pos& to, float radius, const void* userDataFilter = nullptr) const
    {
        return m_world.CastSphere(from, to, radius, userDataFilter);
    }

    JoltShapeCastHit CastCapsule(const float3pos& from, const float3pos& to, float radius, float halfHeight, const quatrot& rotation, const void* userDataFilter = nullptr) const
    {
        return m_world.CastCapsule(from, to, radius, halfHeight, rotation, userDataFilter);
    }

    JoltPhysicsWorld& GetWorld() { return m_world; }
    const JoltPhysicsWorld& GetWorld() const { return m_world; }
    bool IsInitialized() const { return m_initialized; }

private:
    CPhysicsManager() = default;
    ~CPhysicsManager() { Shutdown(); }
    CPhysicsManager(const CPhysicsManager&) = delete;
    CPhysicsManager& operator=(const CPhysicsManager&) = delete;

private:
    bool m_initialized = false;
    JoltPhysicsWorld m_world;
};
