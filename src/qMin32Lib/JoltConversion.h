#pragma once

#include <Jolt/Jolt.h>
#include <Jolt/Math/Vec3.h>
#include <Jolt/Math/Quat.h>
#include "JoltTypes.h"

namespace JoltConvert
{
    inline JPH::Vec3 ToVec3(const float3pos& v) { return JPH::Vec3(v.x, v.y, v.z); }
    inline JPH::Quat ToQuat(const quatrot& q) { return JPH::Quat(q.x, q.y, q.z, q.w); }
    inline float3pos FromVec3(const JPH::Vec3& v) { return { v.GetX(), v.GetY(), v.GetZ() }; }
    inline quatrot FromQuat(const JPH::Quat& q) { return { q.GetX(), q.GetY(), q.GetZ(), q.GetW() }; }
    inline JPH::RVec3 ToRVec3(const float3pos& v) { return JPH::RVec3(v.x, v.y, v.z); }
    inline float3pos FromRVec3(const JPH::RVec3& v) { return { (float)v.GetX(), (float)v.GetY(), (float)v.GetZ() }; }
}
