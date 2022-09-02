#pragma once

#include <nui/reactive.hpp>

#include <tuple>
#include <utility>
#include <concepts>

// FIXME: remove me
#include <iostream>

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

        template <typename... Parameters>
        struct ClearChildrenSideEffect
        {};
        template <typename T, typename... Parameters>
        struct ClearChildrenSideEffect<T, Parameters...>
        {
            static void apply(auto& observedValues, auto& appended, auto& weakParent, auto const& weakFunctionWrapped)
            {
                observedValues.emplaceSideEffects(
                    [weak = std::weak_ptr<typename std::decay_t<decltype(appended)>::element_type>{appended},
                     weakParent,
                     sameFunctionWrapped = weakFunctionWrapped.lock()]() -> bool {
                        if (auto parent = weakParent.lock(); parent)
                        {
                            if (auto shared = weak.lock(); shared)
                                shared->clearChildren();
                            else
                                return false;
                            if (sameFunctionWrapped && *sameFunctionWrapped)
                                (*sameFunctionWrapped)(*parent);
                            return false;
                        }
                        return false;
                    });
                ClearChildrenSideEffect<Parameters...>::apply(
                    observedValues, appended, weakParent, weakFunctionWrapped);
            }
        };
        template <>
        struct ClearChildrenSideEffect<>
        {
            static void apply(auto&&, auto&, auto&, auto const&)
            {}
        };
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

        constexpr auto operator()() &&
        {
            return [self = this->clone()](auto& materializedElement) {
                auto& materialized = materializedElement.appendElement(self);
                return materialized;
            };
        }

        template <typename... ObservedValues, std::invocable... GeneratorT>
        constexpr auto operator()(Reactive<ObservedValues...>&& observedValues, GeneratorT&&... generators) &&
        {
            std::cout << "operator reactive\n";

            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    generators = std::make_tuple(std::forward<GeneratorT>(generators)...)](auto& materializedElement) {
                using ElementType = std::decay_t<decltype(materializedElement)>;
                auto sameFunctionWrapped =
                    std::make_shared<std::function<std::shared_ptr<ElementType>(ElementType&)>>();
                // FIXME: mutable outer then move:
                *sameFunctionWrapped =
                    [self,
                     observedValues,
                     generators,
                     weakFunctionWrapped =
                         std::weak_ptr<typename std::decay_t<decltype(sameFunctionWrapped)>::element_type>{
                             sameFunctionWrapped}](auto& materializedElement) {
                        std::cout << "sameFuctionWrapped\n";

                        auto& appended = materializedElement.appendElement(self);
                        auto weakElement = materializedElement.weak_from_this();
                        std::apply(
                            [&appended,
                             &observedValues,
                             weakFunctionWrapped = std::move(weakFunctionWrapped),
                             weakMaterialized = weakElement](auto&&... generators) {
                                std::apply(
                                    [&appended,
                                     &observedValues,
                                     weakFunctionWrapped = std::move(weakFunctionWrapped),
                                     weakMaterialized = std::move(weakMaterialized)]<typename... GeneratedType>(
                                        GeneratedType&&...) mutable {
                                        Detail::ClearChildrenSideEffect<GeneratedType...>::apply(
                                            observedValues, appended, weakMaterialized, weakFunctionWrapped);
                                    },
                                    std::make_tuple(
                                        std::weak_ptr<typename decltype(generators()(*appended))::element_type>(
                                            generators()(*appended))...));
                            },
                            generators);
                        return appended;
                    };
                return (*sameFunctionWrapped)(materializedElement);
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