#ifndef __INC_REF_H__
#define __INC_REF_H__

#include "ReferenceObject.h"

#include <cassert>
#include <utility>

template <typename T>
class CRef
{
public:
    struct FClear
    {
        void operator()(CRef<T>& ref) const
        {
            ref.Clear();
        }
    };

public:
    CRef() noexcept = default;

    CRef(std::nullptr_t) noexcept
    {
    }

    explicit CRef(T* object)
    {
        Attach(object);
    }

    explicit CRef(CReferenceObject* object)
    {
        Attach(static_cast<T*>(object));
    }

    CRef(const CRef& other)
    {
        Attach(other.m_object);
    }

    CRef(CRef&& other) noexcept
        : m_object(other.m_object)
    {
        other.m_object = nullptr;
    }

    ~CRef()
    {
        Clear();
    }

    CRef& operator=(std::nullptr_t) noexcept
    {
        Clear();
        return *this;
    }

    CRef& operator=(T* object)
    {
        SetPointer(object);
        return *this;
    }

    CRef& operator=(CReferenceObject* object)
    {
        SetPointer(static_cast<T*>(object));
        return *this;
    }

    CRef& operator=(const CRef& other)
    {
        if (this != &other)
            SetPointer(other.m_object);

        return *this;
    }

    CRef& operator=(CRef&& other) noexcept
    {
        if (this != &other)
        {
            Clear();
            m_object = other.m_object;
            other.m_object = nullptr;
        }

        return *this;
    }

    void Clear() noexcept
    {
        T* old = m_object;
        m_object = nullptr;

        if (old)
            old->Release();
    }

    void SetPointer(T* object)
    {
        if (m_object == object)
            return;

        if (object)
            object->AddReference();

        T* old = m_object;
        m_object = object;

        if (old)
            old->Release();
    }

    void Attach(T* object)
    {
        Clear();
        m_object = object;

        if (m_object)
            m_object->AddReference();
    }

    T* Detach() noexcept
    {
        T* object = m_object;
        m_object = nullptr;
        return object;
    }

    bool IsNull() const noexcept
    {
        return m_object == nullptr;
    }

    explicit operator bool() const noexcept
    {
        return m_object != nullptr;
    }

    T* GetPointer() const noexcept
    {
        return m_object;
    }

    T& operator*() const
    {
        assert(m_object != nullptr);
        return *m_object;
    }

    T* operator->() const
    {
        assert(m_object != nullptr);
        return m_object;
    }

private:
    T* m_object = nullptr;
};

#endif
