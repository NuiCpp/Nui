#pragma once

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
            onExit_();
        }
        void disarm()
        {
            onExit_ = [] {};
        }

      private:
        std::function<void()> onExit_;
    };
    template <typename T>
    ScopeExit(T) -> ScopeExit<T>;
}