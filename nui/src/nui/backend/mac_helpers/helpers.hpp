#pragma once

#include "mac_includes.hpp"

namespace Nui::MacOs
{
    template <typename Result, typename Callable, typename... Args>
    Result invoke(Callable callable, Args... args) noexcept
    {
        return reinterpret_cast<Result (*)(Args...)>(callable)(args...);
    }

    // Calls objc_msgSend.
    template <typename Result, typename... Args>
    Result msg_send(Args... args) noexcept
    {
        return invoke<Result>(objc_msgSend, args...);
    }

    inline id operator"" _cls(const char* s, std::size_t)
    {
        return reinterpret_cast<id>(objc_getClass(s));
    }
    inline SEL operator"" _sel(const char* s, std::size_t)
    {
        return sel_registerName(s);
    }
    inline id operator"" _str(const char* s, std::size_t)
    {
        return msg_send<id>("NSString"_cls, "stringWithUTF8String:"_sel, s);
    }
}