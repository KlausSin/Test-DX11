#include "StdAfx.h"
#include "ResourceManager.h"

#include "EterBase/CRC32.h"
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

CFileLoaderThread CResourceManager::ms_loadingThread;

CResourceManager::CResourceManager()
    : m_pTextureCache(std::make_unique<CTextureCache>(512))
{
    ms_loadingThread.Create(0);
}

CResourceManager::~CResourceManager()
{
    DestroyDeletingList();
    Destroy();
    ms_loadingThread.Shutdown();
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

DWORD CResourceManager::__GetFileCRC(const char* c_szFileName, const char** c_ppszLowerFileName)
{
    thread_local std::string normalized;
    normalized = NormalizeFileName(c_szFileName);

    if (c_ppszLowerFileName)
        *c_ppszLowerFileName = normalized.c_str();

    return normalized.empty() ? 0u : GetCRC32(normalized.c_str(), normalized.size());
}

void CResourceManager::LoadStaticCache(const char* c_szFileName)
{
    CResource* pkRes = GetResourcePointer(c_szFileName);
    if (!pkRes)
    {
        Lognf(1, "CResourceManager::LoadStaticCache %s - FAILED", c_szFileName ? c_szFileName : "");
        return;
    }

    const DWORD key = __GetFileCRC(c_szFileName);
    std::lock_guard lock(m_ResourceMapMutex);

    if (m_pCacheMap.contains(key))
        return;

    pkRes->AddReference();
    m_pCacheMap.emplace(key, pkRes);
}

void CResourceManager::ProcessBackgroundLoading()
{
    std::vector<std::string> toRequest;

    {
        std::lock_guard lock(m_ResourceMapMutex);
        for (auto it = m_RequestMap.begin(); it != m_RequestMap.end();)
        {
            const DWORD crc = it->first;
            if (isResourcePointerData(crc) || m_WaitingMap.contains(crc))
            {
                it = m_RequestMap.erase(it);
                continue;
            }

            toRequest.push_back(it->second);
            m_WaitingMap.emplace(crc, it->second);
            it = m_RequestMap.erase(it);
        }
    }

    for (const auto& fileName : toRequest)
        ms_loadingThread.Request(fileName);

    CFileLoaderThread::TData* pData = nullptr;
    while (ms_loadingThread.Fetch(&pData))
    {
        std::unique_ptr<CFileLoaderThread::TData> data(pData);
        CResource* pResource = GetResourcePointer(data->stFileName.c_str());

        if (pResource && pResource->IsEmpty())
        {
            pResource->OnLoad(static_cast<int>(data->File.size()), data->File.data());
            pResource->AddReferenceOnly();

            std::lock_guard lock(m_ResourceMapMutex);
            m_pResRefDecreaseWaitingMap.emplace(NowMS(), pResource);
        }

        std::lock_guard lock(m_ResourceMapMutex);
        m_WaitingMap.erase(__GetFileCRC(data->stFileName.c_str()));
    }

    const DWORD current = NowMS();
    std::lock_guard lock(m_ResourceMapMutex);
    for (auto it = m_pResRefDecreaseWaitingMap.begin(); it != m_pResRefDecreaseWaitingMap.end();)
    {
        if (current - it->first <= ReferenceDecreaseWaitTimeMS)
        {
            ++it;
            continue;
        }

        it->second->Release();
        it = m_pResRefDecreaseWaitingMap.erase(it);
    }
}

void CResourceManager::PushBackgroundLoadingSet(std::set<std::string>& LoadingSet)
{
    std::lock_guard lock(m_ResourceMapMutex);
    for (const auto& fileName : LoadingSet)
    {
        const DWORD crc = __GetFileCRC(fileName.c_str());
        if (!isResourcePointerData(crc))
            m_RequestMap.emplace(crc, fileName);
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

CResource* CResourceManager::InsertResourcePointer(DWORD dwFileCRC, CResource* pResource)
{
    if (!pResource)
        return nullptr;

    std::lock_guard lock(m_ResourceMapMutex);
    auto [it, inserted] = m_pResMap.emplace(dwFileCRC, pResource);
    if (!inserted)
    {
        TraceError("CResource::InsertResourcePointer: %s is already registered\n", pResource->GetFileName());
        delete pResource;
    }

    return it->second;
}

CResource* CResourceManager::GetTypeResourcePointer(const char* c_szFileName, int iType)
{
    if (!c_szFileName || !*c_szFileName)
    {
        assert(c_szFileName && *c_szFileName);
        return nullptr;
    }

    const char* normalizedCStr = nullptr;
    const DWORD crc = __GetFileCRC(c_szFileName, &normalizedCStr);

    if (CResource* pResource = FindResourcePointer(crc))
        return pResource;

    const std::string normalizedFileName = normalizedCStr ? normalizedCStr : "";

    ResourceFactory factory = nullptr;
    {
        std::lock_guard lock(m_ResourceMapMutex);
        factory = FindFactory(normalizedFileName, iType);
    }

    if (!factory)
    {
        TraceError("ResourceManager::GetResourcePointer: NOT SUPPORT FILE %s", normalizedFileName.c_str());
        return nullptr;
    }

    return InsertResourcePointer(crc, factory(normalizedFileName.c_str()));
}

CResource* CResourceManager::GetResourcePointer(const char* c_szFileName)
{
    return GetTypeResourcePointer(c_szFileName, -1);
}

CResource* CResourceManager::FindResourcePointer(DWORD dwFileCRC)
{
    std::lock_guard lock(m_ResourceMapMutex);
    auto it = m_pResMap.find(dwFileCRC);
    return it != m_pResMap.end() ? it->second : nullptr;
}

bool CResourceManager::isResourcePointerData(DWORD dwFileCRC)
{
    std::lock_guard lock(m_ResourceMapMutex);
    auto it = m_pResMap.find(dwFileCRC);
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
