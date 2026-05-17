#if __has_include("StdAfx.h")
#include "StdAfx.h"
#endif

#include "GameRulesAdapter.h"
#include "UserInterface/PythonBackground.h"

float GameRulesAdapter::GetLegacyHeight(float x, float y) const
{
    return CPythonBackground::Instance().GetHeight(x, y);
}

bool GameRulesAdapter::IsNoWalk(float x, float y) const
{
    unsigned char attr = 0;
    if (!CPythonBackground::Instance().GetAttr(x, y, &attr))
        return false;

    return (attr & 0x01) != 0;
}

bool GameRulesAdapter::IsBlockedAttr(float x, float y) const
{
    unsigned char attr = 0;
    if (!CPythonBackground::Instance().GetAttr(x, y, &attr))
        return false;

    return (attr & 0x01) != 0;
}
