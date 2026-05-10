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

class CTextureCache;

template <class T>
class CSingleton;

class CResourceManager : public CSingleton<CResourceManager>
{
public:
    using ResourceFactory = CResource * (*)(const char*);
    using TResourcePointerMap = std::unordered_map<DWORD, CResource*>;
    using TResourceNewFunctionPointerMap = std::unordered_map<std::string, ResourceFactory>;
    using TResourceNewFunctionByTypePointerMap = std::unordered_map<int, ResourceFactory>;
    using TResourceDeletingMap = std::map<CResource*, DWORD>;
    using TResourceRequestMap = std::unordered_map<DWORD, std::string>;
    using TResourceRefDecreaseWaitingMap = std::multimap<DWORD, CResource*>;

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

    CResource* InsertResourcePointer(DWORD dwFileCRC, CResource* pResource);
    CResource* FindResourcePointer(DWORD dwFileCRC);
    CResource* GetResourcePointer(const char* c_szFileName);
    CResource* GetTypeResourcePointer(const char* c_szFileName, int iType = -1);

    bool isResourcePointerData(DWORD dwFileCRC);

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

    DWORD __GetFileCRC(const char* c_szFileName, const char** c_pszLowerFile = nullptr);
    std::string NormalizeFileName(const char* c_szFileName) const;
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

    static CFileLoaderThread ms_loadingThread;
    std::unique_ptr<CTextureCache> m_pTextureCache;

    mutable std::recursive_mutex m_ResourceMapMutex;
};

extern int g_iLoadingDelayTime;
