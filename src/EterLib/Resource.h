#pragma once

#include "ReferenceObject.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>
#include "qMin32Lib/AssetTypes.h"

class CResource : public CReferenceObject
{
public:
    using TType = uint32_t;

    enum EState : uint8_t
    {
        STATE_EMPTY,
        STATE_ERROR,
        STATE_EXIST,
        STATE_LOAD,
        STATE_FREE
    };

public:
    explicit CResource(const char* c_szFileName);
    ~CResource() override = default;

    CResource(const CResource&) = delete;
    CResource& operator=(const CResource&) = delete;

    void Clear();
    void Load();
    void Reload();

    static TType StringToType(const char* c_szType);
    static TType Type();
    static void SetDeleteImmediately(bool isSet = false) noexcept;

    int ConvertPathName(const char* c_szPathName, char* pszRetPathName, int retLen);

    virtual bool CreateDeviceObjects();
    virtual void DestroyDeviceObjects();

    bool IsData() const noexcept;
    bool IsEmpty() const;
    bool IsType(TType type);

    DWORD GetLoadCostMilliSecond() const noexcept { return m_dwLoadCostMiliiSecond; }
    const char* GetFileName() const noexcept { return m_stFileName.c_str(); }
    const std::string& GetFileNameString() const noexcept { return m_stFileName; }
    EState GetState() const noexcept { return me_state; }

    virtual bool OnLoad(int iSize, const void* c_pvBuf) = 0;

protected:
    void SetFileName(const char* c_szFileName);

    virtual void OnClear() = 0;
    virtual bool OnIsEmpty() const = 0;
    virtual bool OnIsType(TType type);

    void OnConstruct() override;
    void OnSelfDestruct() override;

protected:
    std::string m_stFileName;
    DWORD m_dwLoadCostMiliiSecond{ 0 };
    EState me_state{ STATE_EMPTY };

protected:
    static bool ms_bDeleteImmediately;

public:
    bool LoadFromMemory(const void* data, size_t size);
    void MarkQueued() noexcept;
    void MarkLoading() noexcept;
    AssetId GetAssetId() const noexcept { return m_assetId; }
    size_t GetMemorySize() const noexcept { return m_memorySize; }

protected:
    AssetId m_assetId = 0;
    size_t m_memorySize = 0;
};
