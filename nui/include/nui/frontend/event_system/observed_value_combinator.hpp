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
        constexpr ObservedValueCombinatorBase(ObservedValues const&... observedValues)
            : observedValues_{observedValues...}
        {}
        constexpr ObservedValueCombinatorBase(std::tuple<ObservedValues const&...> observedValues)
            : observedValues_{std::move(observedValues)}
        {}

        constexpr void attachEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto const& observed) {
                observed.attachEvent(eventId);
            });
        }

        constexpr void attachOneshotEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto const& observed) {
                observed.attachOneshotEvent(eventId);
            });
        }

        constexpr void unattachEvent(auto eventId) const
        {
            tupleForEach(observedValues_, [eventId](auto const& observed) {
                observed.unattachEvent(eventId);
            });
        }

        std::tuple<ObservedValues const&...> const& observedValues() &
        {
            return observedValues_;
        }

        std::tuple<ObservedValues const&...>&& observedValues() &&
        {
            return std::move(const_cast<std::tuple<ObservedValues const&...>&>(observedValues_));
        }

      protected:
        const std::tuple<ObservedValues const&...> observedValues_;
    };
    template <typename... ObservedValues>
    ObservedValueCombinatorBase(std::tuple<ObservedValues const&...>) -> ObservedValueCombinatorBase<ObservedValues...>;
    template <typename... ObservedValues>
    ObservedValueCombinatorBase(ObservedValues const&...) -> ObservedValueCombinatorBase<ObservedValues...>;

    template <typename... ObservedValues>
    class ObservedValueCombinator;

    template <typename RendererType, typename... ObservedValues>
    class ObservedValueCombinatorWithGenerator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        constexpr ObservedValueCombinatorWithGenerator(
            std::tuple<ObservedValues const&...> observedValues,
            RendererType generator)
            : ObservedValueCombinatorBase<ObservedValues...>{std::move(observedValues)}
            , generator_{std::move(generator)}
        {}

        ObservedValueCombinator<ObservedValues...> split() &&
        {
            return ObservedValueCombinator<ObservedValues...>{std::move(this->observedValues_)};
        }

        constexpr auto generate() const
        {
            return generator_();
        }

        RendererType generator() const&
        {
            return generator_;
        }
        RendererType generator() &&
        {
            return std::move(generator_);
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
    ObservedValueCombinator(ObservedValues const&...) -> ObservedValueCombinator<ObservedValues...>;

    template <typename... ObservedValues>
    ObservedValueCombinator<ObservedValues...> observe(ObservedValues const&... observedValues)
    {
        return ObservedValueCombinator(observedValues...);
    }
}