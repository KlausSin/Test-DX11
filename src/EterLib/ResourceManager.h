#pragma once

#include "Resource.h"
#include "FileLoaderThread.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>
#include "qMin32Lib/AsyncFileSystem.h"

class CTextureCache;

template <class T>
class CSingleton;

class CResourceManager : public CSingleton<CResourceManager>
{
public:
    //using TResourcePointerMap = std::unordered_map<DWORD, CResource*>;
    //using TResourceRequestMap = std::unordered_map<DWORD, std::string>;

    using TResourcePointerMap = std::unordered_map<std::string, CResource*>;
    using TResourceRequestMap = std::unordered_map<std::string, AssetPriority>;
    using ResourceFactory = CResource * (*)(const char*);
    using TResourceNewFunctionPointerMap = std::unordered_map<std::string, ResourceFactory>;
    using TResourceNewFunctionByTypePointerMap = std::unordered_map<int, ResourceFactory>;
    using TResourceDeletingMap = std::map<CResource*, DWORD>;
    using TResourceRefDecreaseWaitingMap = std::multimap<DWORD, CResource*>;

    template<typename T>
    T* GetTyped(const char* c_szFileName)
    {
        return static_cast<T*>(GetResourcePointer(c_szFileName));
    }

public:
    CResourceManager();
    ~CResourceManager() override;

    CResourceManager(const CResourceManager&) = delete;
    CResourceManager& operator=(const CResourceManager&) = delete;

    void LoadStaticCache(const char* c_szFileName);
    void DestroyDeletingList();
    void Destroy();

    void BeginThreadLoading();
    void EndThreadLoading();

    CResource* InsertResourcePointer(const std::string& key, CResource* pResource);
    CResource* FindResourcePointer(const std::string& key);
    CResource* GetResourcePointer(const char* c_szFileName);
    CResource* GetTypeResourcePointer(const char* c_szFileName, int iType = -1);

    bool isResourcePointerData(const char* c_szFileName);

    void RegisterResourceNewFunctionPointer(const char* c_szFileExt, ResourceFactory pResNewFunc);
    void RegisterResourceNewFunctionByTypePointer(int iType, ResourceFactory pNewFunc);

    void DumpFileListToTextFile(const char* c_szFileName);
    bool IsFileExist(const char* c_szFileName);

    void Update();
    void ReserveDeletingResource(CResource* pResource);

    void ProcessBackgroundLoading();
    void PushBackgroundLoadingSet(std::set<std::string>& LoadingSet);

    CTextureCache* GetTextureCache() noexcept { return m_pTextureCache.get(); }

protected:
    void __DestroyDeletingResourceMap();
    void __DestroyResourceMap();
    void __DestroyCacheMap();

    bool isResourcePointerDataByKey(const std::string& key);

    std::string NormalizeFileName(const char* c_szFileName) const;
    std::string MakeKey(const char* c_szFileName) const;

    ResourceFactory FindFactory(const std::string& normalizedFileName, int iType);

protected:
    TResourcePointerMap m_pCacheMap;
    TResourcePointerMap m_pResMap;
    TResourceNewFunctionPointerMap m_pResNewFuncMap;
    TResourceNewFunctionByTypePointerMap m_pResNewFuncByTypeMap;
    TResourceDeletingMap m_ResourceDeletingMap;
    TResourceRequestMap m_RequestMap;
    TResourceRequestMap m_WaitingMap;
    TResourceRefDecreaseWaitingMap m_pResRefDecreaseWaitingMap;

    std::unique_ptr<CTextureCache> m_pTextureCache;

    mutable std::recursive_mutex m_ResourceMapMutex;

public:
    void StartupAsync(uint32_t ioThreads = 1);
    void ShutdownAsync();
    void PreloadResource(const char* c_szFileName, AssetPriority priority = AssetPriority::Normal);

protected:
    AsyncFileSystem m_asyncFiles;
};

extern int g_iLoadingDelayTime;
