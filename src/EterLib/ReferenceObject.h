#pragma once

#include <atomic>
#include <cassert>
#include <cstdint>

class CReferenceObject
{
public:
    CReferenceObject() noexcept = default;
    virtual ~CReferenceObject() = default;

    CReferenceObject(const CReferenceObject&) = delete;
    CReferenceObject& operator=(const CReferenceObject&) = delete;

    void AddReference()
    {
        const auto oldCount = m_refCount.fetch_add(1, std::memory_order_acq_rel);
        if (oldCount == 0)
        {
            m_destructed.store(false, std::memory_order_release);
            OnConstruct();
        }
    }

    void AddReferenceOnly() noexcept
    {
        m_refCount.fetch_add(1, std::memory_order_acq_rel);
    }

    void Release()
    {
        const auto oldCount = m_refCount.load(std::memory_order_acquire);
        assert(oldCount > 0);

        if (oldCount > 1)
        {
            m_refCount.fetch_sub(1, std::memory_order_acq_rel);
            return;
        }

        int32_t expected = 1;
        if (m_refCount.compare_exchange_strong(expected, 0, std::memory_order_acq_rel))
        {
            assert(!m_destructed.load(std::memory_order_acquire));
            OnSelfDestruct();
        }
    }

    int32_t GetReferenceCount() const noexcept
    {
        return m_refCount.load(std::memory_order_acquire);
    }

    bool canDestroy() const noexcept
    {
        return GetReferenceCount() <= 0;
    }

protected:
    virtual void OnConstruct()
    {
        m_destructed.store(false, std::memory_order_release);
    }

    virtual void OnSelfDestruct()
    {
        m_destructed.store(true, std::memory_order_release);
        delete this;
    }

private:
    std::atomic<int32_t> m_refCount{ 0 };
    std::atomic_bool m_destructed{ false };
};
