#pragma once

#include <optional>
#include <functional>

namespace Nui
{
    template <typename ValueT>
    class Lazy
    {
      public:
        Lazy() = default;
        Lazy(Lazy const&) = default;
        Lazy(Lazy&&) = default;
        Lazy& operator=(Lazy const&) = default;
        Lazy& operator=(Lazy&&) = default;

        template <typename FuncT>
        Lazy(FuncT&& func)
            : value_{}
            , obtainer_{std::forward<FuncT>(func)}
        {}

        explicit operator bool() const
        {
            return value_.has_value();
        }

        bool hasValue() const
        {
            return value_.has_value();
        }

        bool tryObtainValue() const
        {
            if (!value_)
                value_ = obtainer_();
            return hasValue();
        }

        std::optional<ValueT> const& value() const&
        {
            if (!value_)
                value_ = obtainer_();
            return value_;
        }

        std::optional<ValueT>&& value() &&
        {
            if (!value_)
                value_ = obtainer_();
            return std::move(value_);
        }

        std::optional<ValueT> value() const&&
        {
            if (!value_)
                value_ = obtainer_();
            return value_;
        }

        std::optional<ValueT>& value() &
        {
            if (!value_)
                value_ = obtainer_();
            return value_;
        }

      private:
        std::optional<ValueT> value_{};
        std::function<std::optional<ValueT>()> obtainer_{};
    };
}