#pragma once

#include <functional>
#include <memory>
#include <any>

namespace Nui
{
    struct EventImpl
    {
        virtual bool call(std::size_t eventId) const = 0;
        virtual bool valid() const = 0;
        virtual ~EventImpl() = default;
    };

    struct TwoFunctorEventImpl : public EventImpl
    {
        TwoFunctorEventImpl(std::function<bool(std::size_t eventId)> action, std::function<bool()> valid)
            : action_{std::move(action)}
            , valid_{std::move(valid)}
        {}

        bool call(std::size_t eventId) const override
        {
            return action_(eventId);
        }

        bool valid() const override
        {
            return valid_();
        }

      private:
        std::function<bool(std::size_t eventId)> action_;
        std::function<bool()> valid_;
    };

    class Event
    {
      public:
        Event(
            std::function<bool(std::size_t eventId)> action,
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
        bool operator()(std::size_t eventId) const
        {
            return impl_->call(eventId);
        }

      private:
        std::unique_ptr<EventImpl> impl_;
    };
}