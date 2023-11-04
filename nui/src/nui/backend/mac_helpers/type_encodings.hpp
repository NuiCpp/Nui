#pragma once

#include "mac_includes.hpp"

#include <string>
#include <vector>
#include <tuple>

namespace Nui::MacOs
{
    template <typename T>
    struct TypeEncoding;

#define NUI_TYPE_ENCODING_SPECIALIZATION(type, encodingString) \
    template <> \
    struct TypeEncoding<type> \
    { \
        static std::string encoding() noexcept \
        { \
            return encodingString; \
        } \
    };

    NUI_TYPE_ENCODING_SPECIALIZATION(char, "c")
    NUI_TYPE_ENCODING_SPECIALIZATION(unsigned char, "C")
    NUI_TYPE_ENCODING_SPECIALIZATION(short, "s")
    NUI_TYPE_ENCODING_SPECIALIZATION(unsigned short, "S")
    NUI_TYPE_ENCODING_SPECIALIZATION(int, "i")
    NUI_TYPE_ENCODING_SPECIALIZATION(unsigned int, "I")
    NUI_TYPE_ENCODING_SPECIALIZATION(long, "l")
    NUI_TYPE_ENCODING_SPECIALIZATION(unsigned long, "L")
    NUI_TYPE_ENCODING_SPECIALIZATION(long long, "q")
    NUI_TYPE_ENCODING_SPECIALIZATION(unsigned long long, "Q")
    NUI_TYPE_ENCODING_SPECIALIZATION(float, "f")
    NUI_TYPE_ENCODING_SPECIALIZATION(double, "d")
    NUI_TYPE_ENCODING_SPECIALIZATION(bool, "B")
    NUI_TYPE_ENCODING_SPECIALIZATION(char*, "*")
    NUI_TYPE_ENCODING_SPECIALIZATION(const char*, "*")
    NUI_TYPE_ENCODING_SPECIALIZATION(void, "v")

    template <typename T>
    struct TypeEncoding<T*>
    {
        static std::string encoding()
        {
            return "^" + TypeEncoding<T>::encoding();
        }
    };

    template <>
    struct TypeEncoding<id>
    {
        static std::string encoding()
        {
            return "@";
        }
    };

    template <>
    struct TypeEncoding<Class>
    {
        static std::string encoding()
        {
            return "#";
        }
    };

    template <>
    struct TypeEncoding<SEL>
    {
        static std::string encoding()
        {
            return ":";
        }
    };

    template <typename T>
    struct TypeEncoding<std::vector<T>>
    {
        static std::string encoding()
        {
            return "[" + TypeEncoding<T>::encoding() + "]";
        }
    };

    template <typename... T>
    struct TypeEncoding<std::tuple<T...>>
    {
        static std::string encoding()
        {
            return "(" + (TypeEncoding<T>::encoding() + ...) + ")";
        }
    };

    template <typename ReturnT, typename... Args>
    struct TypeEncoding<ReturnT(Args...)>
    {
        static std::string encoding()
        {
            return TypeEncoding<ReturnT>::encoding() + (TypeEncoding<Args>::encoding() + ...);
        }
    };
    template <typename ReturnT, typename... Args>
    struct TypeEncoding<ReturnT (*)(Args...)>
    {
        static std::string encoding()
        {
            return TypeEncoding<ReturnT(Args...)>::encoding();
        }
    };

    template <typename T>
    auto encodeType()
    {
        return TypeEncoding<T>::encoding();
    }
}