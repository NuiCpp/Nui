#pragma once

#include <utility>
#include <type_traits>
#include <vector>

namespace emscripten
{
    class val
    {
      public:
        template <typename T>
        val(T)
        {}
        val(val const&)
        {}
        val(val&&)
        {}
        val()
        {}

        val& operator=(val const&)
        {
            return *this;
        }

        val& operator=(val&&)
        {
            return *this;
        }

        val operator[](char const*)
        {
            return {};
        }

        val operator[](char const*) const
        {
            return {};
        }

        val operator[](int)
        {
            return {};
        }

        val operator[](int) const
        {
            return {};
        }

        val operator[](val)
        {
            return {};
        }

        val operator[](val) const
        {
            return {};
        }

        template <typename Ret, typename... List>
        Ret call(char const*, List... args)
        {
            if constexpr (std::is_same_v<Ret, void>)
            {
                return;
            }
            else
            {
                return {};
            }
        }

        static val global(char const*)
        {
            return {};
        }

        static val array()
        {
            return {};
        }

        static val object()
        {
            return {};
        }

        static val u8string()
        {
            return {};
        }

        static val u16string()
        {
            return {};
        }

        static val undefined()
        {
            return {};
        }

        static val null()
        {
            return {};
        }

        val module_property(char const*)
        {
            return {};
        }

        bool hasOwnProperty(char const*) const
        {
            return false;
        }

        template <typename... List>
        val new_(List&&... args)
        {
            return {};
        }
        void delete_(std::string const&){};

        void set(char const*, val const&)
        {}
        void set(val, val const&)
        {}

        template <typename... List>
        val operator()(List&&... args)
        {
            return {};
        }

        template <typename T>
        T as() const
        {
            return {};
        }

        bool isNull() const
        {
            return false;
        }

        bool isUndefined() const
        {
            return false;
        }

        val typeOf() const
        {
            return {};
        }

        val await() const
        {
            return {};
        }
    };

    template <typename T>
    std::vector<T> vecFromJSArray(val const& v)
    {
        return {};
    }

    template <typename T>
    std::vector<T> convertJSArrayToNumberVector(val const& v)
    {
        return {};
    }
}