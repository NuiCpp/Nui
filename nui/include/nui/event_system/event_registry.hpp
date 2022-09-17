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

        /**
         * @brief Returns a pointer to the selected event (only valid until the next activation or event execution). May
         * return nullptr when the event id was not found.
         *
         * @param id
         * @return auto*
         */
        auto* activateEvent(EventIdType id)
        {
            return registry_.select(id);
        }

        void executeEvent(EventIdType id)
        {
            return registry_.deselect(id, [](SelectablesRegistry<Event>::ItemWithId const& itemWithId) -> bool {
                if (!itemWithId.item)
                    return false;
                return itemWithId.item.value()(itemWithId.id);
            });
        }

        void executeActiveEvents()
        {
            registry_.deselectAll([](SelectablesRegistry<Event>::ItemWithId const& itemWithId) -> bool {
                if (!itemWithId.item)
                    return false;
                return itemWithId.item.value()(itemWithId.id);
            });
        }

      private:
        SelectablesRegistry<Event> registry_;
    };
}