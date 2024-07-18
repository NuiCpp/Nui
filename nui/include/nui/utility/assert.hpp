#pragma once

#include <nui/frontend/val.hpp>

#include <stdexcept>

namespace Nui
{
    inline void assertImpl(bool condition, const char* message)
    {
        if (!condition)
        {
#ifdef __cpp_exceptions
            throw std::runtime_error(message);
#else
            Nui::val::global("console").call<void>("error", message);
#endif
        }
    }

#ifdef NDEBUG
#    define NUI_ASSERT(condition, message)
#else
#    define NUI_ASSERT(condition, message) assertImpl(condition, message)
#endif
}