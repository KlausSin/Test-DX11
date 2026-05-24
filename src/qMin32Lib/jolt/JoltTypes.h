#pragma once

#include <cstdint>

struct float3pos
{
    float x, y, z;
};

struct quatrot
{
    float x, y, z, w;
};

struct Transform
{
    float3pos position;
    quatrot rotation;
};

struct RaycastHit
{
    bool hit = false;
    float fraction = 0.0f;
    uint32_t bodyID = 0;
    void* userData = nullptr;
    float3pos position = { 0.0f, 0.0f, 0.0f };
    float3pos normal = { 0.0f, 0.0f, 0.0f };
};

struct JoltSphereQueryResult
{
    bool hit = false;
    float fraction = 0.0f;
    uint32_t bodyID = 0;
    void* userData = nullptr;
    float3pos position = { 0.0f, 0.0f, 0.0f };
    float3pos normal = { 0.0f, 0.0f, 0.0f };
};

struct JoltShapeCastHit
{
    bool hit = false;
    float fraction = 0.0f;
    uint32_t bodyID = 0;
    void* userData = nullptr;
    float3pos position = { 0.0f, 0.0f, 0.0f };
    float3pos normal = { 0.0f, 0.0f, 0.0f };
};

enum class JoltBodyType : uint8_t
{
    Static,
    Dynamic,
    Kinematic
};
