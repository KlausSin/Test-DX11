#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

namespace JoltObjectLayers
{
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

namespace JoltBroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr uint32_t NUM_LAYERS = 2;
}

class JoltBPLayerInterface final : public JPH::BroadPhaseLayerInterface
{
public:
    JoltBPLayerInterface()
    {
        m_objectToBroadPhase[JoltObjectLayers::NON_MOVING] = JoltBroadPhaseLayers::NON_MOVING;
        m_objectToBroadPhase[JoltObjectLayers::MOVING] = JoltBroadPhaseLayers::MOVING;
    }

    uint32_t GetNumBroadPhaseLayers() const override { return JoltBroadPhaseLayers::NUM_LAYERS; }
    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override { return m_objectToBroadPhase[layer]; }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override
    {
        switch ((JPH::BroadPhaseLayer::Type)layer)
        {
            case (JPH::BroadPhaseLayer::Type)0: return "NON_MOVING";
            case (JPH::BroadPhaseLayer::Type)1: return "MOVING";
            default: return "INVALID";
        }
    }
#endif

private:
    JPH::BroadPhaseLayer m_objectToBroadPhase[JoltObjectLayers::NUM_LAYERS];
};

class JoltObjectVsBroadPhaseLayerFilter final : public JPH::ObjectVsBroadPhaseLayerFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override
    {
        switch (layer1)
        {
            case JoltObjectLayers::NON_MOVING: return layer2 == JoltBroadPhaseLayers::MOVING;
            case JoltObjectLayers::MOVING: return true;
            default: return false;
        }
    }
};

class JoltObjectLayerPairFilter final : public JPH::ObjectLayerPairFilter
{
public:
    bool ShouldCollide(JPH::ObjectLayer layer1, JPH::ObjectLayer layer2) const override
    {
        switch (layer1)
        {
            case JoltObjectLayers::NON_MOVING: return layer2 == JoltObjectLayers::MOVING;
            case JoltObjectLayers::MOVING: return true;
            default: return false;
        }
    }
};
