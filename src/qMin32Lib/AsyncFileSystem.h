#pragma once

#include "AssetTypes.h"
#include "PackLib/PackManager.h"
#include <condition_variable>
#include <fstream>
#include <thread>

struct AsyncFileRequest
{
    AssetId id = 0;
    std::string path;
    AssetPriority priority = AssetPriority::Normal;
    uint64_t order = 0;
};

struct AsyncFileResult
{
    AssetId id = 0;
    std::string path;
    AssetBlob blob;
    bool success = false;
};

class AsyncFileSystem
{
public:
    ~AsyncFileSystem() { Stop(); }

    void Start(uint32_t workers = 1)
    {
        Stop();
        m_stop = false;
        workers = std::max(1u, workers);
        for (uint32_t i = 0; i < workers; ++i)
            m_workers.emplace_back([this] { WorkerLoop(); });
    }

    void Stop()
    {
        {
            std::lock_guard lock(m_mutex);
            m_stop = true;
        }
        m_cv.notify_all();
        for (auto& worker : m_workers)
            if (worker.joinable())
                worker.join();
        m_workers.clear();
        Clear();
    }

    void Request(std::string_view path, AssetPriority priority = AssetPriority::Normal)
    {
        AsyncFileRequest req;
        req.path = NormalizeAssetPath(path);
        req.id = MakeAssetId(req.path);
        req.priority = priority;
        req.order = m_order.fetch_add(1, std::memory_order_relaxed);
        {
            std::lock_guard lock(m_mutex);
            m_requests.push(std::move(req));
        }
        m_cv.notify_one();
    }

    bool Fetch(AsyncFileResult& result)
    {
        std::lock_guard lock(m_resultMutex);
        if (m_results.empty())
            return false;
        result = std::move(m_results.front());
        m_results.pop();
        return true;
    }

private:
    struct Compare
    {
        bool operator()(const AsyncFileRequest& a, const AsyncFileRequest& b) const noexcept
        {
            if (a.priority != b.priority)
                return static_cast<uint8_t>(a.priority) < static_cast<uint8_t>(b.priority);
            return a.order > b.order;
        }
    };

    void WorkerLoop()
    {
        for (;;)
        {
            AsyncFileRequest req;
            {
                std::unique_lock lock(m_mutex);
                m_cv.wait(lock, [this] { return m_stop || !m_requests.empty(); });
                if (m_stop && m_requests.empty())
                    return;
                req = std::move(const_cast<AsyncFileRequest&>(m_requests.top()));
                m_requests.pop();
            }

            AsyncFileResult result;
            result.id = req.id;
            result.path = std::move(req.path);
            result.success = Read(result.path, result.blob);
            {
                std::lock_guard lock(m_resultMutex);
                m_results.push(std::move(result));
            }
        }
    }

    static bool Read(const std::string& path, AssetBlob& out)
    {
        TPackFile packFile;
        if (CPackManager::Instance().GetFile(path.c_str(), packFile))
        {
            out.data.resize(packFile.size());
            if (!out.data.empty())
                std::memcpy(out.data.data(), packFile.data(), packFile.size());
            return true;
        }

        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file)
            return false;
        const auto size = file.tellg();
        if (size < 0)
            return false;
        out.data.resize(static_cast<size_t>(size));
        file.seekg(0, std::ios::beg);
        if (!out.data.empty())
            file.read(reinterpret_cast<char*>(out.data.data()), static_cast<std::streamsize>(out.data.size()));
        return file.good() || out.data.empty();
    }

    void Clear()
    {
        std::lock_guard lock1(m_mutex);
        std::lock_guard lock2(m_resultMutex);
        m_requests = {};
        m_results = {};
    }

    mutable std::mutex m_mutex;
    std::condition_variable m_cv;
    std::priority_queue<AsyncFileRequest, std::vector<AsyncFileRequest>, Compare> m_requests;
    std::vector<std::thread> m_workers;
    bool m_stop = false;

    std::mutex m_resultMutex;
    std::queue<AsyncFileResult> m_results;
    std::atomic_uint64_t m_order{0};
};
