#pragma once

#include <functional>
#include <memory>
#include <vector>

namespace Nui
{
    struct EventImpl
    {
        virtual bool call(std::size_t eventId) = 0;
        virtual bool valid() const = 0;
        virtual ~EventImpl() = default;
    };

    struct TwoFunctorEventImpl : public EventImpl
    {
        TwoFunctorEventImpl(std::function<bool(std::size_t eventId)> action, std::function<bool()> valid)
            : action_{std::move(action)}
            , valid_{std::move(valid)}
        {}

        bool call(std::size_t eventId) override
        {
            auto result = action_(eventId);
            return result;
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
        Event(Event const&) = delete;
        Event(Event&&) = default;
        Event& operator=(Event const&) = delete;
        Event& operator=(Event&&) = default;

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