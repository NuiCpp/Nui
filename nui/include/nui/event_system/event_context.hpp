#pragma once

#include <nui/event_system/event_registry.hpp>

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
     * @brief This object can be copied with low cost having share semantics.
     */
    class EventContext
    {
      public:
        using EventIdType = EventRegistry::EventIdType;
        constexpr static auto invalidEventId = EventRegistry::invalidEventId;

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
        void removeEvent(EventIdType id)
        {
            impl_->eventRegistry().removeEvent(id);
        }
        auto activateEvent(EventIdType id)
        {
            return impl_->eventRegistry().activateEvent(id);
        }
        auto activateAfterEffect(EventIdType id)
        {
            return impl_->eventRegistry().activateAfterEffect(id);
        }

        /**
         * @brief Executes all currently active events. This will apply all changes caused by modified Observed<T> to
         * the DOM. Will also execute listen() callbacks that were registered on the modified Observed<T>.
         */
        void executeActiveEventsImmediately()
        {
            impl_->eventRegistry().executeActiveEvents();
        }

        /**
         * @brief Alias for executeActiveEventsImmediately.
         */
        void sync()
        {
            impl_->eventRegistry().executeActiveEvents();
        }

        /**
         * @brief Executes the event with the given id if it was active.
         */
        void executeEvent(EventIdType id)
        {
            impl_->eventRegistry().executeEvent(id);
        }
        EventIdType registerAfterEffect(Event event)
        {
            return impl_->eventRegistry().registerAfterEffect(std::move(event));
        }

        /**
         * @brief Cleans up events from the event registry where the associated dom objects are no longer alive.
         */
        void cleanInvalidEvents()
        {
            impl_->eventRegistry().cleanInvalidEvents();
        }
        void removeAfterEffect(EventIdType id)
        {
            impl_->eventRegistry().removeAfterEffect(id);
        }

        /**
         * @brief Resets the event registry, removing all events. This will break your page!, so it should
         * be used with caution. Mainly useful for testing.
         */
        void reset()
        {
            impl_->eventRegistry().clear();
        }

        /**
         * @brief Can be used to check if events are currently being executed. This is useful to avoid infinite
         * recursion.
         */
        bool isExecutingEvents() const
        {
            return impl_->eventRegistry().isExecutingEvents();
        }

        /**
         * @brief This function can be used to delay the execution of a function until after all currently active events
         * have been executed. This is useful to avoid infinite recursion when you want to trigger an event from within
         * an event handler.
         */
        void delayToAfterProcessing(std::function<void()> func)
        {
            impl_->eventRegistry().delayToAfterProcessing(std::move(func));
        }

      private:
        std::shared_ptr<EventEngine> impl_;
    };

    extern thread_local EventContext globalEventContext;
}