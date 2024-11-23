#pragma once

#include <nui/utility/unique_identifier.hpp>

#include <functional>
#include <utility>

namespace Nui
{
    template <typename EF>
    class ScopeExit
    {
      public:
        ScopeExit(EF&& func)
            : onExit_(std::forward<EF>(func))
        {}
        ~ScopeExit()
        {
            if (onExit_)
                onExit_();
        }
        ScopeExit(ScopeExit&& other)
            : onExit_(std::move(other.onExit_))
        {
            other.onExit_ = {};
        }
        ScopeExit& operator=(ScopeExit&& other)
        {
            if (this != &other)
            {
                onExit_ = std::move(other.onExit_);
                other.onExit_ = {};
            }
            return *this;
        }
        ScopeExit(const ScopeExit&) = delete;
        ScopeExit& operator=(const ScopeExit&) = delete;
        void disarm()
        {
            onExit_ = {};
        }

      private:
        std::function<void()> onExit_;
    };
    template <typename T>
    ScopeExit(T) -> ScopeExit<T>;

    namespace Detail
    {
        struct MakeScopeExitImpl
        {
            template <typename FunctionT>
            auto operator->*(FunctionT&& fn) const
            {
                return ScopeExit<FunctionT>(std::forward<FunctionT>(fn));
            }
        };
    }

#define NUI_ON_SCOPE_EXIT auto NUI_UNIQUE_IDENTIFIER = ::Nui::Detail::MakeScopeExitImpl{}->*[&]
}