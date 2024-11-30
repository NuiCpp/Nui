#pragma once

#include <nui/utility/unique_identifier.hpp>

#include <functional>
#include <type_traits>
#include <utility>

namespace Nui
{
    class ScopeExit
    {
      public:
        template <typename FunctionT>
        requires std::is_nothrow_invocable_v<FunctionT>
        explicit ScopeExit(FunctionT&& func)
            : onExit_(std::forward<FunctionT>(func))
        {}
        ~ScopeExit()
        {
            if (onExit_)
                onExit_();
        }
        ScopeExit(ScopeExit&& other) = delete;
        ScopeExit& operator=(ScopeExit&& other) = delete;
        ScopeExit(const ScopeExit&) = delete;
        ScopeExit& operator=(const ScopeExit&) = delete;
        void disarm()
        {
            onExit_ = {};
        }

      private:
        std::function<void()> onExit_;
    };

    namespace Detail
    {
        struct MakeScopeExitImpl
        {
            template <typename FunctionT>
            auto operator->*(FunctionT&& fn) const
            {
                return ScopeExit{std::forward<FunctionT>(fn)};
            }
        };
    }

#define NUI_ON_SCOPE_EXIT auto NUI_UNIQUE_IDENTIFIER = ::Nui::Detail::MakeScopeExitImpl{}->*[&]() noexcept
}