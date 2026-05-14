#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <cstdint>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Collision/Shape/MeshShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>

#include "JoltTypes.h"

class JoltShapeCache
{
public:
    static JoltShapeCache& Instance()
    {
        static JoltShapeCache instance;
        return instance;
    }

    void Clear()
    {
        m_boxShapes.clear();
        m_sphereShapes.clear();
        m_capsuleShapes.clear();
        m_meshShapes.clear();
        m_heightFieldShapes.clear();
    }

    JPH::ShapeRefC GetBoxShape(const float3pos& halfSize)
    {
        uint64_t key = MakeBoxKey(halfSize);

        auto it = m_boxShapes.find(key);
        if (it != m_boxShapes.end())
            return it->second;

        JPH::ShapeRefC shape = new JPH::BoxShape(JPH::Vec3(halfSize.x, halfSize.y, halfSize.z));
        m_boxShapes.emplace(key, shape);
        return shape;
    }

    JPH::ShapeRefC GetSphereShape(float radius)
    {
        uint32_t key = FloatKey(radius);

        auto it = m_sphereShapes.find(key);
        if (it != m_sphereShapes.end())
            return it->second;

        JPH::ShapeRefC shape = new JPH::SphereShape(radius);
        m_sphereShapes.emplace(key, shape);
        return shape;
    }

    JPH::ShapeRefC GetCapsuleShape(float radius, float halfHeight)
    {
        uint64_t key = MakeCapsuleKey(radius, halfHeight);

        auto it = m_capsuleShapes.find(key);
        if (it != m_capsuleShapes.end())
            return it->second;

        JPH::ShapeRefC shape = new JPH::CapsuleShape(halfHeight, radius);
        m_capsuleShapes.emplace(key, shape);
        return shape;
    }

    JPH::ShapeRefC GetMeshShape(const std::string& name, const JPH::VertexList& vertices, const JPH::IndexedTriangleList& triangles)
    {
        auto it = m_meshShapes.find(name);
        if (it != m_meshShapes.end())
            return it->second;

        JPH::MeshShapeSettings settings(vertices, triangles);
        JPH::ShapeSettings::ShapeResult result = settings.Create();

        if (result.HasError())
            return nullptr;

        JPH::ShapeRefC shape = result.Get();
        m_meshShapes.emplace(name, shape);
        return shape;
    }

    JPH::ShapeRefC GetHeightFieldShape(const std::string& name, const std::vector<float>& samples, uint32_t size, float scaleX, float scaleY, float scaleZ)
    {
        auto it = m_heightFieldShapes.find(name);
        if (it != m_heightFieldShapes.end())
            return it->second;

        JPH::HeightFieldShapeSettings settings(
            samples.data(),
            JPH::Vec3::sZero(),
            JPH::Vec3(scaleX, scaleY, scaleZ),
            size
        );

        JPH::ShapeSettings::ShapeResult result = settings.Create();

        if (result.HasError())
            return nullptr;

        JPH::ShapeRefC shape = result.Get();
        m_heightFieldShapes.emplace(name, shape);
        return shape;
    }

private:
    JoltShapeCache() = default;
    ~JoltShapeCache() = default;

    JoltShapeCache(const JoltShapeCache&) = delete;
    JoltShapeCache& operator=(const JoltShapeCache&) = delete;

private:
    static uint32_t FloatKey(float value)
    {
        union
        {
            float f;
            uint32_t u;
        } data;

        data.f = value;
        return data.u;
    }

    static uint64_t MakeCapsuleKey(float radius, float halfHeight)
    {
        return ((uint64_t)FloatKey(radius) << 32) | FloatKey(halfHeight);
    }

    static uint64_t MakeBoxKey(const float3pos& value)
    {
        uint64_t x = FloatKey(value.x);
        uint64_t y = FloatKey(value.y);
        uint64_t z = FloatKey(value.z);

        return (x * 73856093ull) ^ (y * 19349663ull) ^ (z * 83492791ull);
    }

private:
    std::unordered_map<uint64_t, JPH::ShapeRefC> m_boxShapes;
    std::unordered_map<uint32_t, JPH::ShapeRefC> m_sphereShapes;
    std::unordered_map<uint64_t, JPH::ShapeRefC> m_capsuleShapes;
    std::unordered_map<std::string, JPH::ShapeRefC> m_meshShapes;
    std::unordered_map<std::string, JPH::ShapeRefC> m_heightFieldShapes;
};