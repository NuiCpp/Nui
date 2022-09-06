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