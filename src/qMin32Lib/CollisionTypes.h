#pragma once

#include <cstdint>
#include "JoltTypes.h"

enum class CollisionKind : uint8_t
{
    Unknown = 0,
    Terrain,
    Object,
    Tree,
    Character,
    Mob,
    NPC,
    Dynamic,
    Trigger,
    NoWalk
};

enum class MoveBlockReason : uint8_t
{
    None = 0,
    NoWalk,
    Slope,
    Object,
    Character,
    GroundMissing
};

struct MoveResult
{
    float3pos position = { 0.0f, 0.0f, 0.0f };
    bool blocked = false;
    bool grounded = false;
    MoveBlockReason reason = MoveBlockReason::None;
};

class IGameRules
{
public:
    virtual ~IGameRules() = default;

    virtual float GetLegacyHeight(float x, float y) const = 0;
    virtual bool IsNoWalk(float x, float y) const { return false; }
    virtual bool IsBlockedAttr(float x, float y) const { return false; }
};

using MetinJoltCollisionKind = CollisionKind;
using MetinJoltMoveBlockReason = MoveBlockReason;
using MetinJoltMoveResult = MoveResult;
using IMetinJoltGameRules = IGameRules;
