#pragma once
#include <vector>
#include <deque>
#include <unordered_set>
#include <cstdint>
#include "JoltTypes.h"
#include "JoltPhysicsConfig.h"

class CBaseCollisionInstance;

class JoltCollisionStreamer
{
public:
    static JoltCollisionStreamer& Instance();

    void SetConfig(const JoltPhysicsConfig& config);
    const JoltPhysicsConfig& GetConfig() const;

    void Register(CBaseCollisionInstance* instance);
    void Unregister(CBaseCollisionInstance* instance);
    void Clear();

    void SetCenter(const float3pos& center);
    const float3pos& GetCenter() const;

    void Update(float dt);
    void QueueCreate(CBaseCollisionInstance* instance);
    void QueueDestroy(CBaseCollisionInstance* instance);

    bool CanImmediateCreate();
    void ResetImmediateCreateBudget();

private:
    JoltCollisionStreamer() = default;
    float DistanceSq2D(const float3pos& a, const float3pos& b) const;
    void ProcessQueues();

private:
    JoltPhysicsConfig m_config;
    float3pos m_center = { 0.0f, 0.0f, 0.0f };
    std::vector<CBaseCollisionInstance*> m_instances;
    std::deque<CBaseCollisionInstance*> m_createQueue;
    std::deque<CBaseCollisionInstance*> m_destroyQueue;
    std::unordered_set<CBaseCollisionInstance*> m_createSet;
    std::unordered_set<CBaseCollisionInstance*> m_destroySet;
    uint32_t m_immediateCreatesThisFrame = 0;
};
