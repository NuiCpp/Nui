#pragma once

#include <nui/frontend/event_system/event_context.hpp>
#include <nui/utility/tuple_for_each.hpp>
#include <nui/concepts.hpp>

#include <tuple>

namespace Nui
{
    template <typename... ObservedValues>
    class ObservedValueCombinatorBase
    {
      public:
        constexpr ObservedValueCombinatorBase(ObservedValues&... observedValues)
            : observedValues_{observedValues...}
        {}
        constexpr ObservedValueCombinatorBase(std::tuple<ObservedValues&...> observedValues)
            : observedValues_{std::move(observedValues)}
        {}

        constexpr void attachEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto& observed) {
                observed.attachEvent(eventId);
            });
        }

        constexpr void attachOneshotEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto& observed) {
                observed.attachOneshotEvent(eventId);
            });
        }

        constexpr void unattachEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto& observed) {
                observed.unattachEvent(eventId);
            });
        }

        std::tuple<ObservedValues&...> const& observedValues() &
        {
            return observedValues_;
        }

        std::tuple<ObservedValues&...>&& observedValues() &&
        {
            return std::move(const_cast<std::tuple<ObservedValues&...>&>(observedValues_));
        }

      protected:
        const std::tuple<ObservedValues&...> observedValues_;
    };
    template <typename... ObservedValues>
    ObservedValueCombinatorBase(std::tuple<ObservedValues&...>) -> ObservedValueCombinatorBase<ObservedValues...>;
    template <typename... ObservedValues>
    ObservedValueCombinatorBase(ObservedValues&...) -> ObservedValueCombinatorBase<ObservedValues...>;

    template <typename RendererType, typename... ObservedValues>
    class ObservedValueCombinatorWithGenerator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        constexpr ObservedValueCombinatorWithGenerator(
            std::tuple<ObservedValues&...> observedValues,
            RendererType generator)
            : ObservedValueCombinatorBase<ObservedValues...>{std::move(observedValues)}
            , generator_{std::move(generator)}
        {}

        constexpr auto operator()() const
        {
            return generator_();
        }

        RendererType generator() const
        {
            return generator_;
        }

      private:
        const RendererType generator_;
    };

    template <typename... ObservedValues>
    class ObservedValueCombinator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        using ObservedValueCombinatorBase<ObservedValues...>::ObservedValueCombinatorBase;
        using ObservedValueCombinatorBase<ObservedValues...>::observedValues_;

        template <typename RendererType>
        requires std::invocable<RendererType>
        constexpr ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...>
        generate(RendererType&& generator)
        {
            return ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...>{
                observedValues_, std::forward<RendererType>(generator)};
        }
    };
    template <typename... ObservedValues>
    ObservedValueCombinator(ObservedValues&...) -> ObservedValueCombinator<ObservedValues...>;

    template <typename... ObservedValues>
    ObservedValueCombinator<ObservedValues...> observe(ObservedValues&... observedValues)
    {
        return ObservedValueCombinator(observedValues...);
    }
}