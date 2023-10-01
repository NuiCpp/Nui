#pragma once

#include <nui/frontend/event_system/event_registry.hpp>

#include <memory>

namespace Nui
{
    class EventEngine
    {
      public:
        EventEngine() = default;
        EventEngine(const EventEngine&) = default;
        EventEngine(EventEngine&&) = default;
        EventEngine& operator=(const EventEngine&) = default;
        EventEngine& operator=(EventEngine&&) = default;
        virtual ~EventEngine() = default;

        virtual EventRegistry& eventRegistry() = 0;
    };

    class DefaultEventEngine : public EventEngine
    {
      public:
        EventRegistry& eventRegistry() override
        {
            return eventRegistry_;
        }

      private:
        EventRegistry eventRegistry_;
    };

    /**
     * @brief This object can be copied with low cost.
     */
    class EventContext
    {
      public:
        using EventIdType = EventRegistry::EventIdType;

        EventContext()
            : impl_{std::make_shared<DefaultEventEngine>()}
        {}
        EventContext(EventContext const&) = default;
        EventContext(EventContext&&) = default;
        EventContext& operator=(EventContext const&) = default;
        EventContext& operator=(EventContext&&) = default;
        ~EventContext() = default;

        EventIdType registerEvent(Event event)
        {
            return impl_->eventRegistry().registerEvent(std::move(event));
        }
        auto activateEvent(EventIdType id)
        {
            return impl_->eventRegistry().activateEvent(id);
        }
        void executeActiveEventsImmediately()
        {
            impl_->eventRegistry().executeActiveEvents();
        }
        void executeEvent(EventIdType id)
        {
            impl_->eventRegistry().executeEvent(id);
        }
        EventIdType registerAfterEffect(Event event)
        {
            return impl_->eventRegistry().registerAfterEffect(std::move(event));
        }
        void cleanInvalidEvents()
        {
            impl_->eventRegistry().cleanInvalidEvents();
        }

      private:
        std::shared_ptr<EventEngine> impl_;
    };

    extern thread_local EventContext globalEventContext;
}