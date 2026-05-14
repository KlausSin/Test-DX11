#pragma once

#include "JoltPhysicsWorld.h"

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
        if (m_initialized)
            m_world.Step(dt);
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
