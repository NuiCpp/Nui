#pragma once

#include <nui/data_structures/selecteables_registry.hpp>
#include <nui/utility/visit_overloaded.hpp>
#include <nui/event_system/event.hpp>

#include <functional>
#include <limits>

namespace Nui
{
    class EventRegistry
    {
      public:
        using EventIdType = SelectablesRegistry<Event>::IdType;
        constexpr static EventIdType invalidEventId = std::numeric_limits<EventIdType>::max();

      public:
        EventRegistry() = default;
        EventRegistry(const EventRegistry&) = delete;
        EventRegistry(EventRegistry&&) = default;
        EventRegistry& operator=(const EventRegistry&) = delete;
        EventRegistry& operator=(EventRegistry&&) = default;
        ~EventRegistry() = default;

        EventIdType registerEvent(Event event)
        {
            return registry_.append(std::move(event));
        }

        bool activateEvent(EventIdType id)
        {
            return registry_.select(id);
        }

        void executeActiveEvents()
        {
            registry_.deselectAll([](SelectablesRegistry<Event>::ItemWithId const& itemWithId) -> bool {
                if (!itemWithId.item)
                    return false;
                return itemWithId.item.value()();
            });
        }

      private:
        SelectablesRegistry<Event> registry_;
    };
}