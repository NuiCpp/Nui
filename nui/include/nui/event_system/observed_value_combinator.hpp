#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/utility/tuple_for_each.hpp>

#include <tuple>

namespace Nui
{
    template <typename... ObservedValues>
    struct ObservedValueCombinator
    {
      public:
        constexpr ObservedValueCombinator(ObservedValues&... observedValues)
            : observedValues_{observedValues...}
        {}

        constexpr void attachEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto& observed) {
                observed.attachEvent(eventId);
            });
        }

      private:
        const std::tuple<ObservedValues&...> observedValues_;
    };

    template <typename... ObservedValues>
    ObservedValueCombinator<ObservedValues...> observe(ObservedValues&... observedValues)
    {
        return ObservedValueCombinator(observedValues...);
    }
}