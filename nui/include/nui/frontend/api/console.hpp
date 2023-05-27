#pragma once

#include <nui/frontend/utility/val_conversion.hpp>

#include <nui/frontend/val.hpp>

namespace Nui
{
    class Console
    {
      public:
        template <typename... Args>
        static void log(Args&&... args)
        {
            Nui::val::global("console").call<void>("log", convertToVal(std::forward<Args>(args))...);
        }
        template <typename... Args>
        static void error(Args&&... args)
        {
            Nui::val::global("console").call<void>("error", convertToVal(std::forward<Args>(args))...);
        }
        template <typename... Args>
        static void warn(Args&&... args)
        {
            Nui::val::global("console").call<void>("warn", convertToVal(std::forward<Args>(args))...);
        }
        template <typename... Args>
        static void info(Args&&... args)
        {
            Nui::val::global("console").call<void>("info", convertToVal(std::forward<Args>(args))...);
        }
        template <typename... Args>
        static void debug(Args&&... args)
        {
            Nui::val::global("console").call<void>("debug", convertToVal(std::forward<Args>(args))...);
        }
        template <typename... Args>
        static void trace(Args&&... args)
        {
            Nui::val::global("console").call<void>("trace", convertToVal(std::forward<Args>(args))...);
        }
        template <typename... Args>
        static void table(Args&&... args)
        {
            Nui::val::global("console").call<void>("table", convertToVal(std::forward<Args>(args))...);
        }
        static void group()
        {
            Nui::val::global("console").call<void>("group");
        }
        static void groupCollapsed()
        {
            Nui::val::global("console").call<void>("groupCollapsed");
        }
        static void groupEnd()
        {
            Nui::val::global("console").call<void>("groupEnd");
        }
        static void clear()
        {
            Nui::val::global("console").call<void>("clear");
        }
        template <typename T>
        static void assert_(bool condition, T&& message)
        {
            Nui::val::global("console").call<void>("assert", condition, std::forward<T>(message));
        }
    };
}