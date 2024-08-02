#pragma once

#if !defined(__cpp_exceptions) && !defined(__EMSCRIPTEN__)
#    include <nui/frontend/val.hpp>
#else
#    include <iostream>
#endif

#include <stdexcept>

namespace Nui
{
    inline void assertImpl(bool condition, const char* message)
    {
        if (!condition)
        {
#ifdef __cpp_exceptions
            throw std::runtime_error(message);
#elif defined(__EMSCRIPTEN__)
            Nui::val::global("console").call<void>("error", message);
#else
            std::cerr << message << std::endl;
            std::terminate();
#endif
        }
    }

#ifdef NDEBUG
#    define NUI_ASSERT(condition, message)
#else
#    define NUI_ASSERT(condition, message) assertImpl(condition, message)
#endif
}