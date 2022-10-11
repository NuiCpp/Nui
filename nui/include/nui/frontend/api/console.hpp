#pragma once

#include <emscripten/val.h>

namespace Nui
{
    class Console
    {
      public:
        template <typename... Args>
        static void log(Args&&... args)
        {
            emscripten::val::global("console").call<void>("log", std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void error(Args&&... args)
        {
            emscripten::val::global("console").call<void>("error", std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void warn(Args&&... args)
        {
            emscripten::val::global("console").call<void>("warn", std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void info(Args&&... args)
        {
            emscripten::val::global("console").call<void>("info", std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void debug(Args&&... args)
        {
            emscripten::val::global("console").call<void>("debug", std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void trace(Args&&... args)
        {
            emscripten::val::global("console").call<void>("trace", std::forward<Args>(args)...);
        }
        template <typename... Args>
        static void table(Args&&... args)
        {
            emscripten::val::global("console").call<void>("table", std::forward<Args>(args)...);
        }
        static void group()
        {
            emscripten::val::global("console").call<void>("group");
        }
        static void groupCollapsed()
        {
            emscripten::val::global("console").call<void>("groupCollapsed");
        }
        static void groupEnd()
        {
            emscripten::val::global("console").call<void>("groupEnd");
        }
        static void clear()
        {
            emscripten::val::global("console").call<void>("clear");
        }
        template <typename T>
        static void assert_(bool condition, T&& message)
        {
            emscripten::val::global("console").call<void>("assert", condition, std::forward<T>(message));
        }
    };
}