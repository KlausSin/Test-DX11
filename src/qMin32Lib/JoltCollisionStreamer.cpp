#include "JoltCollisionStreamer.h"
#include "EterLib/CollisionData.h"
#include "CPhysicsManager.h"
#include <algorithm>

JoltCollisionStreamer& JoltCollisionStreamer::Instance()
{
    static JoltCollisionStreamer s_instance;
    return s_instance;
}

void JoltCollisionStreamer::SetConfig(const JoltPhysicsConfig& config)
{
    m_config = config;
}

const JoltPhysicsConfig& JoltCollisionStreamer::GetConfig() const
{
    return m_config;
}

void JoltCollisionStreamer::Register(CBaseCollisionInstance* instance)
{
    if (!instance)
        return;

    m_instances.push_back(instance);
}

void JoltCollisionStreamer::Unregister(CBaseCollisionInstance* instance)
{
    if (!instance)
        return;

    m_instances.erase(std::remove(m_instances.begin(), m_instances.end(), instance), m_instances.end());
    m_createSet.erase(instance);
    m_destroySet.erase(instance);
    instance->ReleaseJoltBody();
}

void JoltCollisionStreamer::Clear()
{
    for (CBaseCollisionInstance* instance : m_instances)
    {
        if (instance)
            instance->ReleaseJoltBody();
    }

    m_instances.clear();
    m_createQueue.clear();
    m_destroyQueue.clear();
    m_createSet.clear();
    m_destroySet.clear();
    m_immediateCreatesThisFrame = 0;
}

void JoltCollisionStreamer::SetCenter(const float3pos& center)
{
    m_center = center;
}

const float3pos& JoltCollisionStreamer::GetCenter() const
{
    return m_center;
}

float JoltCollisionStreamer::DistanceSq2D(const float3pos& a, const float3pos& b) const
{
    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    return dx * dx + dy * dy;
}

void JoltCollisionStreamer::QueueCreate(CBaseCollisionInstance* instance)
{
    if (!instance || instance->HasJoltBody())
        return;

    if (m_createSet.insert(instance).second)
        m_createQueue.push_back(instance);
}

void JoltCollisionStreamer::QueueDestroy(CBaseCollisionInstance* instance)
{
    if (!instance || !instance->HasJoltBody())
        return;

    if (m_destroySet.insert(instance).second)
        m_destroyQueue.push_back(instance);
}

bool JoltCollisionStreamer::CanImmediateCreate()
{
    if (m_immediateCreatesThisFrame >= m_config.maxImmediateQueryCreatesPerFrame)
        return false;

    ++m_immediateCreatesThisFrame;
    return true;
}

void JoltCollisionStreamer::ResetImmediateCreateBudget()
{
    m_immediateCreatesThisFrame = 0;
}

void JoltCollisionStreamer::Update(float)
{
    ResetImmediateCreateBudget();

    const float createDistanceSq = m_config.streamCreateDistance * m_config.streamCreateDistance;
    const float destroyDistanceSq = m_config.streamDestroyDistance * m_config.streamDestroyDistance;

    for (CBaseCollisionInstance* instance : m_instances)
    {
        if (!instance)
            continue;

        const float d2 = DistanceSq2D(instance->GetJoltWorldCenter(), m_center);
        if (d2 <= createDistanceSq)
            QueueCreate(instance);
        else if (d2 >= destroyDistanceSq)
            QueueDestroy(instance);
    }

    ProcessQueues();
}

void JoltCollisionStreamer::ProcessQueues()
{
    if (!CPhysicsManager::Instance().IsInitialized())
        return;

    uint32_t destroys = 0;
    while (!m_destroyQueue.empty() && destroys < m_config.maxBodyDestroysPerFrame)
    {
        CBaseCollisionInstance* instance = m_destroyQueue.front();
        m_destroyQueue.pop_front();
        m_destroySet.erase(instance);

        if (instance)
            instance->ReleaseJoltBody();

        ++destroys;
    }

    uint32_t creates = 0;
    while (!m_createQueue.empty() && creates < m_config.maxBodyCreatesPerFrame)
    {
        CBaseCollisionInstance* instance = m_createQueue.front();
        m_createQueue.pop_front();
        m_createSet.erase(instance);

        if (instance && !instance->HasJoltBody())
            instance->EnsureJoltBody(false);

        ++creates;
    }
}
