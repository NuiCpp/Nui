#pragma once

namespace Nui
{
    namespace Detail
    {
        template <typename T>
        T* referenceGObject(T* ptr)
        {
            if (ptr)
                g_object_ref_sink(ptr);
            return ptr;
        }

        template <typename T>
        void unreferenceGObject(T* ptr)
        {
            if (ptr)
                g_object_unref(ptr);
        }
    }

    template <typename T>
    class GObjectReference
    {
      public:
        struct AdoptFlag
        {};

        using value_type = T;
        using pointer_type = value_type*;

        GObjectReference()
            : ptr_{nullptr}
        {}

        explicit GObjectReference(pointer_type ptr)
            : ptr_{ptr}
        {
            if (ptr_)
                Detail::referenceGObject(ptr_);
        }

        GObjectReference(GObjectReference const& other)
            : ptr_{other.ptr_}
        {
            if (ptr_)
                Detail::referenceGObject(ptr_);
        }

        template <typename U>
        GObjectReference(GObjectReference<U> const& other)
            : ptr_{other.get()}
        {
            if (ptr_)
                Detail::referenceGObject(ptr_);
        }

        GObjectReference(GObjectReference&& other)
            : ptr_{other.release()}
        {}

        ~GObjectReference()
        {
            Detail::unreferenceGObject(ptr_);
        }

        void clear()
        {
            using std::swap;
            T* ptr = nullptr;
            swap(ptr_, ptr);
            if (ptr)
                derefGPtr(ptr);
        }

        pointer_type release()
        {
            pointer_type ptr = nullptr;
            using std::swap;
            swap(ptr_, ptr);
            return ptr;
        }

        T* get() const
        {
            return ptr_;
        }
        T& operator*() const
        {
            return *ptr_;
        }
        T* operator->() const
        {
            return ptr_;
        }

        explicit operator bool() const
        {
            return ptr_ != nullptr;
        }
        bool operator!() const
        {
            return ptr_ == nullptr;
        }

        GObjectReference& operator=(GObjectReference const& other)
        {
            pointer_type optr = other.get();
            if (optr)
                Detail::referenceGObject(optr);
            pointer_type ptr = ptr_;
            ptr_ = optr;
            if (ptr)
                Detail::unreferenceGObject(ptr);
            return *this;
        }
        GObjectReference& operator=(GObjectReference&& other)
        {
            GObjectReference(std::move(other)).swap(*this);
            return *this;
        }
        GObjectReference& operator=(pointer_type optr)
        {
            pointer_type ptr = ptr_;
            if (optr)
                Detail::referenceGObject(optr);
            ptr_ = optr;
            if (ptr)
                Detail::unreferenceGObject(ptr);
            return *this;
        }
        template <typename U>
        GObjectReference& operator=(GObjectReference<U> const& other)
        {
            GObjectReference(other).swap(*this);
            return *this;
        }

        void swap(GObjectReference& other)
        {
            using std::swap;
            swap(ptr_, other.ptr_);
        }
        static GObjectReference<T> adoptReference(T* ptr)
        {
            return GObjectReference<T>(ptr, AdoptFlag{});
        }

      private:
        GObjectReference(pointer_type ptr, AdoptFlag)
            : ptr_{ptr}
        {}

      private:
        T* ptr_;
    };

    template <typename T>
    void swap(GObjectReference<T>& lhs, GObjectReference<T>& rhs)
    {
        lhs.swap(rhs);
    }

    template <typename T, typename U>
    bool operator==(GObjectReference<T> const& lhs, GObjectReference<U> const& rhs)
    {
        return lhs.get() == rhs.get();
    }
    template <typename T, typename U>
    bool operator==(GObjectReference<T> const& lhs, T* rhs)
    {
        return lhs.get() == rhs;
    }
    template <typename T, typename U>
    bool operator==(T* lhs, GObjectReference<U> const& rhs)
    {
        return lhs == rhs.get();
    }

    template <typename T, typename U>
    bool operator!=(GObjectReference<T> const& lhs, GObjectReference<U> const& rhs)
    {
        return lhs.get() != rhs.get();
    }
    template <typename T, typename U>
    bool operator!=(GObjectReference<T> const& lhs, T* rhs)
    {
        return lhs.get() != rhs;
    }
    template <typename T, typename U>
    bool operator!=(T* lhs, GObjectReference<U> const& rhs)
    {
        return lhs != rhs.get();
    }

    template <typename T, typename U>
    GObjectReference<T> static_pointer_cast(GObjectReference<U> const& ptr)
    {
        return GObjectReference<T>(static_cast<T*>(ptr.get()));
    }
    template <typename T, typename U>
    GObjectReference<T> dynamic_pointer_cast(GObjectReference<U> const& ptr)
    {
        return GObjectReference<T>(dynamic_cast<T*>(ptr.get()));
    }
}