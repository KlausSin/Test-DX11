#pragma once

#include "CollisionTypes.h"

class GameRulesAdapter final : public IGameRules
{
public:
    float GetLegacyHeight(float x, float y) const override;
    bool IsNoWalk(float x, float y) const override;
    bool IsBlockedAttr(float x, float y) const override;
};

using MetinJoltGameRulesAdapter = GameRulesAdapter;
