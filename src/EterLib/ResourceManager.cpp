#include "StdAfx.h"
#include "ResourceManager.h"

#include "EterBase/Stl.h"
#include "PackLib/PackManager.h"
#include "TextureCache.h"
#include "GrpImage.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <thread>

int g_iLoadingDelayTime = 1;

namespace
{
    constexpr DWORD DeletingWaitTimeMS = 30000;
    constexpr DWORD DeletingCountPerFrame = 30;
    constexpr DWORD ReferenceDecreaseWaitTimeMS = 30000;

    DWORD NowMS()
    {
        using namespace std::chrono;
        return static_cast<DWORD>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
    }

    std::string LowerExt(const std::string& path)
    {
        const auto pos = path.find_last_of('.');
        if (pos == std::string::npos)
            return {};

        auto ext = path.substr(pos + 1, 8);
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return ext;
    }
}

std::string CResourceManager::MakeKey(const char* c_szFileName) const
{
    return NormalizeFileName(c_szFileName);
}

void CResourceManager::StartupAsync(uint32_t ioThreads)
{
    m_asyncFiles.Start(ioThreads);
}

void CResourceManager::ShutdownAsync()
{
    m_asyncFiles.Stop();
}

void CResourceManager::PreloadResource(const char* c_szFileName, AssetPriority priority)
{
    const std::string key = MakeKey(c_szFileName);
    if (key.empty())
        return;

    CResource* res = FindResourcePointer(key);
    if (!res)
        res = GetResourcePointer(key.c_str());

    if (!res || res->IsData())
        return;

    res->MarkQueued();

    std::lock_guard lock(m_ResourceMapMutex);
    m_RequestMap[key] = priority;
}

CResourceManager::CResourceManager()
    : m_pTextureCache(std::make_unique<CTextureCache>(512))
{
    StartupAsync(1);
}

CResourceManager::~CResourceManager()
{
    ShutdownAsync();

    DestroyDeletingList();
    Destroy();
}

void CResourceManager::BeginThreadLoading()
{
}

void CResourceManager::EndThreadLoading()
{
    for (;;)
    {
        {
            std::lock_guard lock(m_ResourceMapMutex);
            if (m_RequestMap.empty() && m_WaitingMap.empty())
                break;
        }

        ProcessBackgroundLoading();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

std::string CResourceManager::NormalizeFileName(const char* c_szFileName) const
{
    if (!c_szFileName)
        return {};

    std::string out(c_szFileName);
    std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) {
        return ch == '/' ? '\\' : static_cast<char>(std::tolower(ch));
        });
    return out;
}

void CResourceManager::LoadStaticCache(const char* c_szFileName)
{
    CResource* pkRes = GetResourcePointer(c_szFileName);
    if (!pkRes)
    {
        Lognf(1, "CResourceManager::LoadStaticCache %s - FAILED", c_szFileName ? c_szFileName : "");
        return;
    }

    const std::string key = MakeKey(c_szFileName);
    if (key.empty())
        return;

    std::lock_guard lock(m_ResourceMapMutex);

    if (m_pCacheMap.find(key) != m_pCacheMap.end())
        return;

    pkRes->AddReference();
    m_pCacheMap.emplace(key, pkRes);
}

void CResourceManager::ProcessBackgroundLoading()
{
    std::vector<std::pair<std::string, AssetPriority>> requests;
    {
        std::lock_guard lock(m_ResourceMapMutex);
        for (auto it = m_RequestMap.begin(); it != m_RequestMap.end();)
        {
            if (m_WaitingMap.find(it->first) != m_WaitingMap.end())
            {
                ++it;
                continue;
            }
            requests.emplace_back(it->first, it->second);
            m_WaitingMap[it->first] = it->second;
            it = m_RequestMap.erase(it);
        }
    }

    for (const auto& req : requests)
        m_asyncFiles.Request(req.first, req.second);

    AsyncFileResult result;
    while (m_asyncFiles.Fetch(result))
    {
        CResource* res = FindResourcePointer(result.path);

        if (res)
        {
            if (result.success && result.blob.Data() && result.blob.Size() > 0)
                res->LoadFromMemory(result.blob.Data(), result.blob.Size());
            else
                res->LoadFromMemory(nullptr, 0);
        }

        result.blob.Clear();

        std::lock_guard lock(m_ResourceMapMutex);
        m_WaitingMap.erase(result.path);
    }
}

void CResourceManager::PushBackgroundLoadingSet(std::set<std::string>& LoadingSet)
{
    std::lock_guard lock(m_ResourceMapMutex);

    for (const auto& fileName : LoadingSet)
    {
        const std::string key = MakeKey(fileName.c_str());
        if (key.empty())
            continue;

        auto it = m_pResMap.find(key);
        if (it != m_pResMap.end() && it->second && it->second->IsData())
            continue;

        m_RequestMap.emplace(key, AssetPriority::Normal);
    }
}

void CResourceManager::__DestroyCacheMap()
{
    std::lock_guard lock(m_ResourceMapMutex);
    for (auto& [_, pResource] : m_pCacheMap)
        if (pResource)
            pResource->Release();
    m_pCacheMap.clear();
}

void CResourceManager::__DestroyDeletingResourceMap()
{
    std::lock_guard lock(m_ResourceMapMutex);
    Tracenf("CResourceManager::__DestroyDeletingResourceMap %zu", m_ResourceDeletingMap.size());
    for (auto& [pResource, _] : m_ResourceDeletingMap)
        if (pResource)
            pResource->Clear();
    m_ResourceDeletingMap.clear();
}

void CResourceManager::__DestroyResourceMap()
{
    std::lock_guard lock(m_ResourceMapMutex);
    Tracenf("CResourceManager::__DestroyResourceMap %zu", m_pResMap.size());
    for (auto& [_, pResource] : m_pResMap)
        if (pResource)
        {
            pResource->Clear();
            delete pResource;
        }
    m_pResMap.clear();
}

void CResourceManager::DestroyDeletingList()
{
    CResource::SetDeleteImmediately(true);
    __DestroyCacheMap();
    __DestroyDeletingResourceMap();
}

void CResourceManager::Destroy()
{
    __DestroyResourceMap();
}

void CResourceManager::RegisterResourceNewFunctionPointer(const char* c_szFileExt, ResourceFactory pNewFunc)
{
    if (!c_szFileExt || !pNewFunc)
        return;

    std::string ext(c_szFileExt);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });

    std::lock_guard lock(m_ResourceMapMutex);
    m_pResNewFuncMap[ext] = pNewFunc;
}

void CResourceManager::RegisterResourceNewFunctionByTypePointer(int iType, ResourceFactory pNewFunc)
{
    assert(iType >= 0);
    if (!pNewFunc)
        return;

    std::lock_guard lock(m_ResourceMapMutex);
    m_pResNewFuncByTypeMap[iType] = pNewFunc;
}

CResourceManager::ResourceFactory CResourceManager::FindFactory(const std::string& normalizedFileName, int iType)
{
    if (iType != -1)
    {
        auto it = m_pResNewFuncByTypeMap.find(iType);
        return it != m_pResNewFuncByTypeMap.end() ? it->second : nullptr;
    }

    const auto ext = LowerExt(normalizedFileName);
    if (ext.empty())
        return nullptr;

    auto it = m_pResNewFuncMap.find(ext);
    return it != m_pResNewFuncMap.end() ? it->second : nullptr;
}

CResource* CResourceManager::InsertResourcePointer(const std::string& key, CResource* pResource)
{
    if (!pResource)
        return nullptr;
    std::lock_guard lock(m_ResourceMapMutex);
    auto [it, inserted] = m_pResMap.emplace(key, pResource);
    if (!inserted)
        delete pResource;
    return it->second;
}

CResource* CResourceManager::GetTypeResourcePointer(const char* c_szFileName, int iType)
{
    const std::string key = MakeKey(c_szFileName);
    if (key.empty())
        return nullptr;
    if (CResource* res = FindResourcePointer(key))
        return res;
    ResourceFactory factory = nullptr;
    {
        std::lock_guard lock(m_ResourceMapMutex);
        factory = FindFactory(key, iType);
    }
    return factory ? InsertResourcePointer(key, factory(key.c_str())) : nullptr;
}

CResource* CResourceManager::GetResourcePointer(const char* c_szFileName)
{
    return GetTypeResourcePointer(c_szFileName, -1);
}

CResource* CResourceManager::FindResourcePointer(const std::string& key)
{
    std::lock_guard lock(m_ResourceMapMutex);
    auto it = m_pResMap.find(key);
    return it == m_pResMap.end() ? nullptr : it->second;
}

bool CResourceManager::isResourcePointerData(const char* c_szFileName)
{
    const std::string key = MakeKey(c_szFileName);
    if (key.empty())
        return false;

    return isResourcePointerDataByKey(key);
}

bool CResourceManager::isResourcePointerDataByKey(const std::string& key)
{
    std::lock_guard lock(m_ResourceMapMutex);

    auto it = m_pResMap.find(key);
    return it != m_pResMap.end() && it->second && it->second->IsData();
}

struct SDumpData
{
    const char* filename{};
    float KB{};
    DWORD cost{};
};

void CResourceManager::DumpFileListToTextFile(const char* c_szFileName)
{
    std::vector<SDumpData> dumpVector;

    {
        std::lock_guard lock(m_ResourceMapMutex);
        for (const auto& [_, pResource] : m_pResMap)
        {
            if (!pResource || pResource->IsEmpty())
                continue;

            int fileSize = 0;
            const char* filename = pResource->GetFileName();
            const char* ext = std::strrchr(filename, '.');

            if (ext && pResource->IsType(CGraphicImage::Type()) && strnicmp(ext, ".sub", 4))
                fileSize = static_cast<CGraphicImage*>(pResource)->GetWidth() * static_cast<CGraphicImage*>(pResource)->GetHeight() * 4;
            else if (FILE* fp = std::fopen(filename, "rb"))
            {
                std::fseek(fp, 0L, SEEK_END);
                fileSize = static_cast<int>(std::ftell(fp));
                std::fclose(fp);
            }

            dumpVector.push_back({ filename, static_cast<float>(fileSize) / 1024.0f, pResource->GetLoadCostMilliSecond() });
        }
    }

    FILE* fp = std::fopen(c_szFileName, "w");
    if (!fp)
        return;

    std::sort(dumpVector.begin(), dumpVector.end(), [](const SDumpData& a, const SDumpData& b) { return a.KB > b.KB; });

    float totalKB = 0.0f;
    for (const auto& data : dumpVector)
    {
        totalKB += data.KB;
        std::fprintf(fp, "%6.1f %s\n", data.KB, data.filename);
    }
    std::fprintf(fp, "total: %.2fmb\n", totalKB / 1024.0f);

    std::sort(dumpVector.begin(), dumpVector.end(), [](const SDumpData& a, const SDumpData& b) { return a.cost > b.cost; });
    for (const auto& data : dumpVector)
        std::fprintf(fp, "%-4u %s\n", data.cost, data.filename);
    std::fprintf(fp, "total: %.2fmb\n", totalKB / 1024.0f);

    std::fclose(fp);
}

bool CResourceManager::IsFileExist(const char* c_szFileName)
{
    return CPackManager::Instance().IsExist(c_szFileName);
}

void CResourceManager::Update()
{
    const DWORD current = NowMS();
    uint32_t count = 0;

    {
        std::lock_guard lock(m_ResourceMapMutex);
        for (auto it = m_ResourceDeletingMap.begin(); it != m_ResourceDeletingMap.end();)
        {
            CResource* pResource = it->first;
            if (current < it->second)
            {
                ++it;
                continue;
            }

            if (pResource && pResource->canDestroy())
                pResource->Clear();

            it = m_ResourceDeletingMap.erase(it);
            if (++count >= DeletingCountPerFrame)
                break;
        }
    }

    ProcessBackgroundLoading();
}

void CResourceManager::ReserveDeletingResource(CResource* pResource)
{
    if (!pResource)
        return;

    std::lock_guard lock(m_ResourceMapMutex);
    m_ResourceDeletingMap[pResource] = NowMS() + DeletingWaitTimeMS;
}
