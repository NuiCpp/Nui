#pragma once

#include <nui/reactive.hpp>

#include <tuple>
#include <utility>
#include <concepts>

namespace Nui
{
    namespace Detail
    {
        template <typename>
        struct IsTuple : std::false_type
        {};
        template <typename... T>
        struct IsTuple<std::tuple<T...>> : std::true_type
        {};
    }

    template <typename Derived, typename... Attributes>
    class HtmlElement
    {
      public:
        friend class DomElement;

        constexpr HtmlElement(HtmlElement const&) = default;
        constexpr HtmlElement(HtmlElement&&) = default;
        constexpr HtmlElement(std::tuple<Attributes...> attributes)
            : attributes_{std::move(attributes)}
        {}
        template <typename... T>
        constexpr HtmlElement(T&&... attributes)
            : attributes_{std::forward<T>(attributes)...}
        {}

        HtmlElement clone() const
        {
            return {attributes_};
        }

        template <typename... ElementT>
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return [self = this->clone(),
                    children = std::make_tuple(std::forward<ElementT>(elements)...)](auto& materializedElement) {
                auto materialized = materializedElement.appendElement(self);
                materialized->appendElements(children);
                return materialized;
            };
        }

        template <typename... ObservedValues, std::invocable... GeneratorT>
        constexpr auto operator()(Reactive<ObservedValues...>&& observedValues, GeneratorT&&... generators) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    generators = std::make_tuple(std::forward<GeneratorT>(generators)...)](auto& materializedElement) {
                auto appended = materializedElement.appendElement(self);
                std::apply(
                    [&appended, &observedValues](auto&&... generators) {
                        std::apply(
                            [&appended]<typename... GeneratedType>(GeneratedType&&...) {
                                observedValues.emplaceSideEffects([&appended](typename GeneratedType::value_type) {
                                    appended->clearChildren();
                                }...);
                            },
                            std::make_tuple(std::weak_ptr<typename decltype(generators()(*appended))::element_type>(
                                generators()(*appended))...));
                    },
                    generators);
            };
        }

        std::tuple<Attributes...> const& attributes() const
        {
            return attributes_;
        }

      private:
        std::tuple<Attributes...> attributes_;
    };
}