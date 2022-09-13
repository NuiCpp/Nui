#pragma once

#include <functional>
#include <memory>
#include <vector>
#include <any>

namespace Nui
{
    struct EventImpl
    {
        using meta_data_type = std::vector<std::any>;

        virtual bool call(std::size_t eventId) = 0;
        virtual bool valid() const = 0;
        virtual ~EventImpl() = default;
    };

    struct TwoFunctorEventImpl : public EventImpl
    {
        TwoFunctorEventImpl(
            std::function<bool(std::size_t eventId, meta_data_type const&)> action,
            std::function<bool()> valid)
            : action_{std::move(action)}
            , valid_{std::move(valid)}
        {}

        bool call(std::size_t eventId) override
        {
            auto result = action_(eventId, executionMetadata_);
            executionMetadata_.clear();
            return result;
        }

        bool valid() const override
        {
            return valid_();
        }

      private:
        std::function<bool(std::size_t eventId, std::vector<std::any> const&)> action_;
        std::function<bool()> valid_;
        std::vector<std::any> executionMetadata_;
    };

    class Event
    {
      public:
        Event(
            std::function<bool(std::size_t eventId, EventImpl::meta_data_type const&)> action,
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