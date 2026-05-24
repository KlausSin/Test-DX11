#pragma once

#include <cmath>
#include "CPhysicsManager.h"
#include "CollisionTypes.h"
#include "CollisionObject.h"

class CharacterMotor
{
public:
    void Setup(float radius, float halfHeight);
    void SetMaxSlopeDegrees(float degrees);
    void SetGroundProbeHeight(float height);
    void SetGroundProbeDistance(float distance);
    void SetSkinWidth(float width);
    void SetGroundOffset(float offset);
    void SetGameRules(const IGameRules* rules);

    MoveResult Move(const float3pos& currentPosition, const float3pos& desiredDelta) const;

private:
    bool IsBlockingKind(CollisionKind kind) const;
    bool QueryGround(const float3pos& position, float& outZ, float3pos& outNormal) const;
    bool IsSlopeAllowed(const float3pos& normal) const;
    bool CastBlockingCapsule(const float3pos& fromFoot, const float3pos& toFoot) const;

    float3pos ToJoltCenter(const float3pos& foot) const;
    float3pos ToGameFoot(const float3pos& center) const;

private:
    float m_radius = 35.0f;
    float m_halfHeight = 90.0f;
    float m_maxSlopeCos = 0.5f;
    float m_groundProbeHeight = 3000.0f;
    float m_groundProbeDistance = 6000.0f;
    float m_skinWidth = 2.0f;
    float m_groundOffset = 0.0f;
    quatrot m_capsuleRotation = { 0.70710678f, 0.0f, 0.0f, 0.70710678f };
    const IGameRules* m_rules = nullptr;
};

using MetinJoltCharacterMotor = CharacterMotor;
