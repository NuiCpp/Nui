#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/event_context.hpp>
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

      protected:
        const std::tuple<ObservedValues&...> observedValues_;
    };
    template <typename... ObservedValues>
    ObservedValueCombinatorBase(std::tuple<ObservedValues&...>) -> ObservedValueCombinatorBase<ObservedValues...>;
    template <typename... ObservedValues>
    ObservedValueCombinatorBase(ObservedValues&...) -> ObservedValueCombinatorBase<ObservedValues...>;

    template <typename GeneratorType, typename... ObservedValues>
    class ObservedValueCombinatorWithGenerator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        constexpr ObservedValueCombinatorWithGenerator(
            std::tuple<ObservedValues&...> observedValues,
            GeneratorType generator)
            : ObservedValueCombinatorBase<ObservedValues...>{std::move(observedValues)}
            , generator_{std::move(generator)}
        {}

        constexpr auto operator()() const
        {
            return generator_();
        }

        GeneratorType generator() const
        {
            return generator_;
        }

      private:
        const GeneratorType generator_;
    };

    template <typename... ObservedValues>
    class ObservedValueCombinator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        using ObservedValueCombinatorBase<ObservedValues...>::ObservedValueCombinatorBase;
        using ObservedValueCombinatorBase<ObservedValues...>::observedValues_;

        template <typename GeneratorType>
        requires std::invocable<GeneratorType>
        constexpr ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...>
        generate(GeneratorType&& generator)
        {
            return ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...>{
                observedValues_, std::forward<GeneratorType>(generator)};
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