#pragma once

#include <nui/observed.hpp>

#include <concepts>

namespace Nui
{

    namespace Detail
    {
        template <typename... ObservedValues, typename... SideEffects>
        constexpr void AttachEffects(std::tuple<ObservedValues...> observed, SideEffects&&... effects)
        {
            (..., observed.template emplaceSideEffect(std::forward<SideEffects>(effects)));
        }
    }

    template <typename... ObservedValues>
    struct Reactive
    {
      public:
        Reactive(ObservedValues&... observedValues)
            : observedValues_{observedValues...}
        {}

        void emplaceSideEffects(auto&&... sideEffect)
        {
            Detail::AttachEffects(observedValues_, std::forward<decltype(sideEffect)>(sideEffect)...);
        }

      private:
        std::tuple<ObservedValues&...> observedValues_;
    };

    template <typename... ObservedValues>
    Reactive<ObservedValues...> observe(ObservedValues&... observedValues)
    {
        return Reactive(observedValues...);
    }
}