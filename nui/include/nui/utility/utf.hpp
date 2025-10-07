#pragma once

#include <utf8.h>

#include <iterator>

namespace Nui
{
    namespace Detail
    {
        template <typename T>
        concept IsStringCharTypeUtf16Compatible = requires(T str) {
            typename T::value_type;
            requires(sizeof(typename T::value_type) > 1);
        };

        template <typename T>
        concept IsStringCharTypeUtf8Compatible = requires(T str) {
            typename T::value_type;
            requires(sizeof(typename T::value_type) == 1);
        };
    }

    template <typename Utf16StringType, typename Utf8StringType>
    requires(Detail::IsStringCharTypeUtf16Compatible<Utf16StringType>) &&
        (Detail::IsStringCharTypeUtf8Compatible<Utf8StringType>)
    Utf16StringType utf8ToUtf16(Utf8StringType const& str)
    {
        Utf16StringType result;
        utf8::utf8to16(str.begin(), str.end(), std::back_inserter(result));
        return result;
    }

    template <typename Utf16StringType, typename Utf8StringType>
    requires(Detail::IsStringCharTypeUtf16Compatible<Utf16StringType>) &&
        (Detail::IsStringCharTypeUtf8Compatible<Utf8StringType>)
    Utf8StringType utf16ToUtf8(Utf16StringType const& str)
    {
        Utf8StringType result;
        utf8::utf16to8(str.begin(), str.end(), std::back_inserter(result));
        return result;
    }
}