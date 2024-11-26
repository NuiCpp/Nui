#pragma once

#include <nui/data_structures/selectables_registry.hpp>
#include <nui/frontend/event_system/event.hpp>
#include <nui/utility/visit_overloaded.hpp>

#include <limits>

namespace Nui
{
    class EventRegistry
    {
      public:
        using RegistryType = SelectablesRegistry<Event>;
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
        RegistryType::SelectionResult activateEvent(EventIdType id)
        {
            return registry_.select(id);
        }

        /**
         * @brief Activate an after effect.
         *
         * @param id
         * @return RegistryType::SelectionResult
         */
        RegistryType::SelectionResult activateAfterEffect(EventIdType id)
        {
            return afterEffects_.select(id);
        }

        /**
         * @brief After effects are used to cause something to happen after all other events have been processed.
         * After effects are executed in indeterminate order.
         *
         * @param event
         * @return EventIdType
         */
        EventIdType registerAfterEffect(Event event)
        {
            return afterEffects_.append(std::move(event));
        }

        void executeEvent(EventIdType id)
        {
            registry_.deselect(id, [](SelectablesRegistry<Event>::ItemWithId const& itemWithId) -> bool {
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
            afterEffects_.deselectAll([](SelectablesRegistry<Event>::ItemWithId const& itemWithId) -> bool {
                if (!itemWithId.item)
                    return false;
                return itemWithId.item.value()(itemWithId.id);
            });
        }

        void cleanInvalidEvents()
        {
            std::vector<EventIdType> invalidIds;
            for (auto const& itemWithId : registry_.rawRange())
            {
                if (!itemWithId.item)
                    continue;
                if (!static_cast<bool>(itemWithId.item.value()))
                    invalidIds.push_back(itemWithId.id);
            }

            for (auto const& id : invalidIds)
                registry_.erase(id);
        }

        void removeAfterEffect(EventIdType id)
        {
            afterEffects_.erase(id);
        }

        void clear()
        {
            registry_.clear();
            afterEffects_.clear();
        }

      private:
        RegistryType registry_;
        RegistryType afterEffects_;
    };
}