#pragma once

#include <functional>

namespace Nui
{
    class OnDestroy
    {
      public:
        OnDestroy(std::function<void()> onDestroy)
            : wasMoved_{false}
            , onDestroy_{std::move(onDestroy)}
        {}
        OnDestroy(OnDestroy const&) = delete;
        OnDestroy(OnDestroy&& other)
            : wasMoved_{other.wasMoved_}
            , onDestroy_{std::move(other.onDestroy_)}
        {
            other.wasMoved_ = true;
        }
        OnDestroy& operator=(OnDestroy const&) = delete;
        OnDestroy& operator=(OnDestroy&& other)
        {
            wasMoved_ = other.wasMoved_;
            onDestroy_ = std::move(other.onDestroy_);
            other.wasMoved_ = true;
            return *this;
        }
        ~OnDestroy()
        {
            if (!wasMoved_ && onDestroy_)
                onDestroy_();
        }

      private:
        bool wasMoved_;
        std::function<void()> onDestroy_;
    };
}