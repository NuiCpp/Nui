#pragma once

#include <locale>
#include <codecvt>
#include <string>

namespace Nui
{
    inline std::wstring widenString(std::string const& str)
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.from_bytes(str);
#pragma clang diagnostic pop
    }

    inline std::string shortenString(std::wstring const& str)
    {
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
        std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
        return converter.to_bytes(str);
#pragma clang diagnostic pop
    }
}