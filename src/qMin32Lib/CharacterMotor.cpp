#include "CharacterMotor.h"

void CharacterMotor::Setup(float radius, float halfHeight)
{
    m_radius = radius;
    m_halfHeight = halfHeight;
}

void CharacterMotor::SetMaxSlopeDegrees(float degrees)
{
    const float radians = degrees * 3.1415926535f / 180.0f;
    m_maxSlopeCos = cosf(radians);
}

void CharacterMotor::SetGroundProbeHeight(float height)
{
    m_groundProbeHeight = height;
}

void CharacterMotor::SetGroundProbeDistance(float distance)
{
    m_groundProbeDistance = distance;
}

void CharacterMotor::SetSkinWidth(float width)
{
    m_skinWidth = width;
}

void CharacterMotor::SetGroundOffset(float offset)
{
    m_groundOffset = offset;
}

void CharacterMotor::SetGameRules(const IGameRules* rules)
{
    m_rules = rules;
}

MoveResult CharacterMotor::Move(const float3pos& currentPosition, const float3pos& desiredDelta) const
{
    MoveResult result;
    result.position = currentPosition;

    float3pos wanted = { currentPosition.x + desiredDelta.x, currentPosition.y + desiredDelta.y, currentPosition.z };

    if (m_rules && (m_rules->IsNoWalk(wanted.x, wanted.y) || m_rules->IsBlockedAttr(wanted.x, wanted.y)))
    {
        result.blocked = true;
        result.reason = MoveBlockReason::NoWalk;
        return result;
    }

    float groundZ = currentPosition.z;
    float3pos groundNormal = { 0.0f, 0.0f, 1.0f };

    if (!QueryGround(wanted, groundZ, groundNormal))
    {
        if (m_rules)
            groundZ = m_rules->GetLegacyHeight(wanted.x, wanted.y);
        else
        {
            result.blocked = true;
            result.reason = MoveBlockReason::GroundMissing;
            return result;
        }
    }

    if (!IsSlopeAllowed(groundNormal))
    {
        result.blocked = true;
        result.reason = MoveBlockReason::Slope;
        return result;
    }

    wanted.z = groundZ + m_groundOffset;

    if (CastBlockingCapsule(currentPosition, wanted))
    {
        float3pos tryX = { wanted.x, currentPosition.y, currentPosition.z };
        float tryXGround = tryX.z;
        float3pos tryXNormal = { 0.0f, 0.0f, 1.0f };

        if (QueryGround(tryX, tryXGround, tryXNormal) && IsSlopeAllowed(tryXNormal))
        {
            tryX.z = tryXGround + m_groundOffset;
            if (!CastBlockingCapsule(currentPosition, tryX))
            {
                result.position = tryX;
                result.grounded = true;
                result.blocked = true;
                result.reason = MoveBlockReason::Object;
                return result;
            }
        }

        float3pos tryY = { currentPosition.x, wanted.y, currentPosition.z };
        float tryYGround = tryY.z;
        float3pos tryYNormal = { 0.0f, 0.0f, 1.0f };

        if (QueryGround(tryY, tryYGround, tryYNormal) && IsSlopeAllowed(tryYNormal))
        {
            tryY.z = tryYGround + m_groundOffset;
            if (!CastBlockingCapsule(currentPosition, tryY))
            {
                result.position = tryY;
                result.grounded = true;
                result.blocked = true;
                result.reason = MoveBlockReason::Object;
                return result;
            }
        }

        result.blocked = true;
        result.reason = MoveBlockReason::Object;
        return result;
    }

    result.position = wanted;
    result.grounded = true;
    return result;
}

bool CharacterMotor::QueryGround(const float3pos& position, float& outZ, float3pos& outNormal) const
{
    if (!CPhysicsManager::Instance().IsInitialized())
        return false;

    const RaycastHit hit = CPhysicsManager::Instance().Raycast(
        { position.x, -position.y, position.z + m_groundProbeHeight },
        { 0.0f, 0.0f, -1.0f },
        m_groundProbeDistance
    );

    if (!hit.hit)
        return false;

    const CollisionObject* object = CollisionObject::FromUserData(hit.userData);
    if (object)
    {
        const CollisionKind kind = object->GetKind();
        if (kind != CollisionKind::Terrain && kind != CollisionKind::Object && kind != CollisionKind::Tree)
            return false;
    }

    outZ = hit.position.z;
    outNormal = hit.normal;
    return true;
}

bool CharacterMotor::IsSlopeAllowed(const float3pos& normal) const
{
    return normal.z >= m_maxSlopeCos;
}

bool CharacterMotor::CastBlockingCapsule(const float3pos& fromFoot, const float3pos& toFoot) const
{
    if (!CPhysicsManager::Instance().IsInitialized())
        return false;

    const float3pos fromCenter = ToJoltCenter(fromFoot);
    const float3pos toCenter = ToJoltCenter(toFoot);

    const JoltShapeCastHit hit = CPhysicsManager::Instance().CastCapsule(fromCenter, toCenter, m_radius, m_halfHeight, m_capsuleRotation);
    if (!hit.hit)
        return false;

    const CollisionObject* object = CollisionObject::FromUserData(hit.userData);
    if (!object)
        return true;

    return IsBlockingKind(object->GetKind());
}

bool CharacterMotor::IsBlockingKind(CollisionKind kind) const
{
    switch (kind)
    {
        case CollisionKind::Object:
        case CollisionKind::Tree:
        case CollisionKind::Character:
        case CollisionKind::Mob:
        case CollisionKind::NPC:
        case CollisionKind::Dynamic:
        case CollisionKind::NoWalk:
            return true;

        case CollisionKind::Terrain:
        case CollisionKind::Trigger:
        case CollisionKind::Unknown:
        default:
            return false;
    }
}

float3pos CharacterMotor::ToJoltCenter(const float3pos& foot) const
{
    return { foot.x, -foot.y, foot.z + m_radius + m_halfHeight + m_skinWidth };
}

float3pos CharacterMotor::ToGameFoot(const float3pos& center) const
{
    return { center.x, -center.y, center.z - m_radius - m_halfHeight - m_skinWidth };
}
