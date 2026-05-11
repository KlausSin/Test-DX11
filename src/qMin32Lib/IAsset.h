#pragma once

#include "AssetTypes.h"

class IAsset
{
public:
    virtual ~IAsset() = default;
    virtual AssetTypeId GetTypeId() const noexcept = 0;
    virtual bool LoadFromMemory(const void* data, size_t size) = 0;
    virtual void Unload() = 0;
    virtual bool IsEmpty() const noexcept = 0;
    virtual size_t GetMemorySize() const noexcept { return 0; }
};
