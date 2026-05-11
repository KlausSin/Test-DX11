#pragma once

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using AssetId = uint32_t;
using AssetTypeId = uint32_t;

inline std::string NormalizeAssetPath(std::string_view path)
{
    std::string out;
    out.reserve(path.size());
    for (char c : path)
        out.push_back(c == '/' ? '\\' : static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    return out;
}

inline uint32_t Fnv1a32(std::string_view text)
{
    uint32_t hash = 2166136261u;
    for (unsigned char c : text)
    {
        hash ^= c;
        hash *= 16777619u;
    }
    return hash;
}

inline AssetId MakeAssetId(std::string_view path)
{
    return Fnv1a32(NormalizeAssetPath(path));
}

inline AssetTypeId MakeAssetTypeId(std::string_view name)
{
    return Fnv1a32(name);
}

enum class AssetState : uint8_t
{
    Empty,
    Queued,
    Loading,
    Loaded,
    Failed,
    Unloaded
};

enum class AssetPriority : uint8_t
{
    Low,
    Normal,
    High,
    Critical
};

struct AssetBlob
{
    std::vector<uint8_t> data;
    bool Empty() const noexcept { return data.empty(); }
    size_t Size() const noexcept { return data.size(); }
    const void* Data() const noexcept { return data.empty() ? nullptr : data.data(); }
    void Clear() { data.clear(); data.shrink_to_fit(); }
};
