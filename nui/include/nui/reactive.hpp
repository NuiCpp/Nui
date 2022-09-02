#pragma once

#include <nui/observed.hpp>

#include <concepts>

namespace Nui
{
    template <typename... ObservedValues>
    struct Reactive
    {
      public:
        constexpr Reactive(ObservedValues&... observedValues)
            : observedValues_{observedValues...}
        {}

        void emplaceSideEffects(auto&&... sideEffect) const
        {
            std::apply(
                [&sideEffect...](auto& observed) {
                    (..., observed.emplaceSideEffect(sideEffect));
                },
                observedValues_);
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