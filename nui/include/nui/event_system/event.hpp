#pragma once

#include <functional>
#include <memory>
#include <any>

namespace Nui
{
    struct EventImpl
    {
        virtual bool call() const = 0;
        virtual bool valid() const = 0;
        virtual ~EventImpl() = default;
    };

    struct TwoFunctorEventImpl : public EventImpl
    {
        TwoFunctorEventImpl(std::function<bool()> action, std::function<bool()> valid)
            : action_{std::move(action)}
            , valid_{std::move(valid)}
        {}

        bool call() const override
        {
            return action_();
        }

        bool valid() const override
        {
            return valid_();
        }

      private:
        std::function<bool()> action_;
        std::function<bool()> valid_;
    };

    template <typename T>
    struct WeakPtrEventImpl : EventImpl
    {
      public:
        WeakPtrEventImpl(std::weak_ptr<T> weak, std::function<bool(std::shared_ptr<T> const& thing)> action)
            : weak_{std::move(weak)}
            , action_{std::move(action)}
        {}

        bool call() const override
        {
            if (auto shared = weak_.lock(); shared)
                return action_(shared);
            return false;
        }

        bool valid() const override
        {
            return !weak_.expired();
        }

      private:
        std::weak_ptr<T> weak_;
        std::function<bool(std::shared_ptr<T> const& thing)> action_;
    };

    class Event
    {
      public:
        Event(
            std::function<bool()> action,
            std::function<bool()> valid =
                [] {
                    return true;
                })
            : impl_{std::make_unique<TwoFunctorEventImpl>(std::move(action), std::move(valid))}
        {}
        template <typename T>
        Event(std::weak_ptr<T> weak, std::function<bool(std::shared_ptr<T> const& thing)> action)
            : impl_{std::make_unique<WeakPtrEventImpl<T>>(std::move(weak), std::move(action))}
        {}

        operator bool() const
        {
            return impl_->valid();
        }
        bool operator()() const
        {
            return impl_->call();
        }

      private:
        std::unique_ptr<EventImpl> impl_;
    };
}