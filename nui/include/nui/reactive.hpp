#pragma once

#include <nui/observed.hpp>
#include <nui/utility/tuple_for_each.hpp>

#include <tuple>

namespace Nui
{
    template <typename... ObservedValues>
    struct Reactive
    {
      public:
        constexpr Reactive(ObservedValues&... observedValues)
            : observedValues_{observedValues...}
        {}

        constexpr void emplaceSideEffects(auto const& sideEffect) const
        {
            tupleForEach(observedValues_, [&sideEffect](auto& observed){
                observed.emplaceSideEffect(sideEffect);
            });
        }

      private:
        const std::tuple<ObservedValues&...> observedValues_;
    };

    template <typename... ObservedValues>
    Reactive<ObservedValues...> observe(ObservedValues&... observedValues)
    {
        return Reactive(observedValues...);
    }
}