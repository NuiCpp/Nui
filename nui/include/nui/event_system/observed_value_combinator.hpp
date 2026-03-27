#pragma once

#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/utility/tuple_for_each.hpp>
#include <nui/utility/overloaded.hpp>
#include <nui/concepts.hpp>
#include <traits/functions.hpp>

#include <tuple>

namespace Nui
{
    template <typename... ObservedValues>
    class ObservedValueCombinatorBase
    {
      public:
        explicit constexpr ObservedValueCombinatorBase(
            Detail::ObservedAddReference_t<ObservedValues>&&... observedValues)
            : observedValues_{std::forward<Detail::ObservedAddReference_t<ObservedValues>>(observedValues)...}
        {}
        explicit constexpr ObservedValueCombinatorBase(
            std::tuple<Detail::ObservedAddReference_t<ObservedValues>...> observedValues)
            : observedValues_{std::move(observedValues)}
        {}

        constexpr void attachEvent(auto eventId) const
        {
            tupleForEach(
                observedValues_,
                Nui::overloaded{
                    [eventId](IsObserved auto const& observed) {
                        observed.attachEvent(eventId);
                    },
                    [eventId](IsWeakObserved auto const& observed) {
                        if (auto shared = observed.lock(); shared)
                            shared->attachEvent(eventId);
                    },
                });
        }

        constexpr void attachOneshotEvent(auto eventId) const
        {
            tupleForEach(
                observedValues_,
                Nui::overloaded{
                    [eventId](IsObserved auto const& observed) {
                        observed.attachOneshotEvent(eventId);
                    },
                    [eventId](IsWeakObserved auto const& observed) {
                        if (auto shared = observed.lock(); shared)
                            shared->attachOneshotEvent(eventId);
                    },
                });
        }

        constexpr void detachEvent(auto eventId) const
        {
            tupleForEach(
                observedValues_,
                Nui::overloaded{
                    [eventId](IsObserved auto const& observed) {
                        observed.detachEvent(eventId);
                    },
                    [eventId](IsWeakObserved auto const& observed) {
                        if (auto shared = observed.lock(); shared)
                            shared->detachEvent(eventId);
                    },
                });
        }

        std::tuple<Detail::ObservedAddReference_t<ObservedValues>...> const& observedValues() &
        {
            return observedValues_;
        }

        std::tuple<Detail::ObservedAddReference_t<ObservedValues>...> const& observedValues() const&
        {
            return observedValues_;
        }

        std::tuple<Detail::ObservedAddReference_t<ObservedValues>...>&& observedValues() &&
        {
            return std::move(observedValues_);
        }

        bool isAnyExpired() const
        {
            const auto isExpired = Nui::overloaded{
                [](IsObserved auto const&) {
                    return false;
                },
                [](IsWeakObserved auto const& observed) {
                    return observed.expired();
                },
            };

            return std::apply(
                [isExpired](auto const&... observed) {
                    return (isExpired(observed) || ...);
                },
                observedValues_);
        }

      protected:
        std::tuple<Detail::ObservedAddReference_t<ObservedValues>...> observedValues_;
    };

    template <typename... ObservedValues>
    class ObservedValueCombinator;

    template <typename RendererType, typename... ObservedValues>
    class ObservedValueCombinatorWithGenerator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        using RendererTypeNoRef = std::decay_t<RendererType>;

        constexpr ObservedValueCombinatorWithGenerator(
            std::tuple<Detail::ObservedAddReference_t<ObservedValues>...> observedValues,
            RendererType generator)
            : ObservedValueCombinatorBase<ObservedValues...>{std::move(observedValues)}
            , generator_{std::move(generator)}
        {}

        ObservedValueCombinator<ObservedValues...> split() &&
        {
            return ObservedValueCombinator<ObservedValues...>{std::move(this->observedValues_)};
        }

        ObservedValueCombinator<ObservedValues...> combinator()
        {
            return ObservedValueCombinator<ObservedValues...>{this->observedValues_};
        }

        constexpr auto value() const
        {
            if constexpr (std::invocable<RendererTypeNoRef>)
                return generator_();
            else
            {
                return std::apply(
                    [&](auto const&... observed) {
                        return generator_([&observed]() {
                            // If weak pointer, lock and unpack:
                            if constexpr (IsWeakObserved<decltype(observed)>)
                            {
                                auto locked = observed.lock();
                                if (locked)
                                    return locked->value();
                                return typename std::decay_t<decltype(observed)>::element_type::observed_type{};
                            }
                            // shared_ptr->value()
                            else if constexpr (IsSharedObserved<decltype(observed)>)
                            {
                                return observed->value();
                            }
                            // regular observed
                            else
                            {
                                return observed.value();
                            }
                        }()...);
                    },
                    this->observedValues_);
            }
        }

        void generate(RendererTypeNoRef generator)
        {
            this->generator_ = std::move(generator);
        }

        RendererTypeNoRef generator() const&
        {
            return generator_;
        }
        RendererTypeNoRef generator() &&
        {
            return std::move(generator_);
        }

      protected:
        RendererTypeNoRef generator_;
    };

    template <typename... ObservedValues>
    class ObservedValueCombinator : public ObservedValueCombinatorBase<ObservedValues...>
    {
      public:
        using ObservedValueCombinatorBase<ObservedValues...>::ObservedValueCombinatorBase;
        using ObservedValueCombinatorBase<ObservedValues...>::observedValues_;

        template <typename RendererType>
        requires(
            std::invocable<RendererType> ||
            std::invocable<std::decay_t<RendererType>, typename std::decay_t<ObservedValues>::observed_type...> ||
            Traits::CallableOfArity<RendererType, sizeof...(ObservedValues)>)
        constexpr ObservedValueCombinatorWithGenerator<std::decay_t<RendererType>, ObservedValues...>
        generate(RendererType&& generator) const&
        {
            return ObservedValueCombinatorWithGenerator<std::decay_t<RendererType>, ObservedValues...>{
                observedValues_, std::forward<RendererType>(generator)};
        }

        template <typename RendererType>
        requires(
            std::invocable<RendererType> ||
            std::invocable<std::decay_t<RendererType>, typename std::decay_t<ObservedValues>::observed_type...> ||
            Traits::CallableOfArity<RendererType, sizeof...(ObservedValues)>)
        constexpr ObservedValueCombinatorWithGenerator<std::decay_t<RendererType>, ObservedValues...>
        generate(RendererType&& generator) &&
        {
            return ObservedValueCombinatorWithGenerator<std::decay_t<RendererType>, ObservedValues...>{
                std::move(observedValues_), std::forward<RendererType>(generator)};
        }
    };
    template <typename... ObservedValues>
    ObservedValueCombinator(ObservedValues&&...)
        -> ObservedValueCombinator<std::decay_t<Detail::ObservedAddReference_t<ObservedValues>>...>;
    template <typename... ObservedValues>
    ObservedValueCombinator(std::tuple<Detail::ObservedAddReference_t<ObservedValues>...>)
        -> ObservedValueCombinator<std::decay_t<Detail::ObservedAddReference_t<ObservedValues>>...>;

    template <typename... ObservedValues>
    requires(IsObservedLike<ObservedValues> && ...)
    ObservedValueCombinator<std::decay_t<Detail::ObservedAddReference_t<ObservedValues>>...>
    observe(ObservedValues&&... observedValues)
    {
        return ObservedValueCombinator<std::decay_t<Detail::ObservedAddReference_t<ObservedValues>>...>{
            std::forward<Detail::ObservedAddReference_t<ObservedValues>>(observedValues)...};
    }

    template <typename... Types>
    struct IsObservedValueCombinatorImpl
    {
        static constexpr bool value = false;
    };

    template <typename... Types>
    struct IsObservedValueCombinatorImpl<ObservedValueCombinator<Types...>>
    {
        static constexpr bool value = true;
    };

    template <typename... Types>
    struct IsObservedValueCombinatorWithGeneratorImpl
    {
        static constexpr bool value = false;
    };

    template <typename... Types>
    struct IsObservedValueCombinatorWithGeneratorImpl<ObservedValueCombinatorWithGenerator<Types...>>
    {
        static constexpr bool value = true;
    };

    template <typename T>
    concept IsObservedValueCombinator = IsObservedValueCombinatorImpl<std::decay_t<T>>::value;

    template <typename T>
    concept IsObservedValueCombinatorWithGenerator = IsObservedValueCombinatorWithGeneratorImpl<std::decay_t<T>>::value;
}