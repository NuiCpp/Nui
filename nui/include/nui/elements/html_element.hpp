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

        template <typename... Parameters>
        struct RefabricationSideEffect
        {};
        template <typename T, typename... Parameters>
        struct RefabricationSideEffect<T, Parameters...>
        {
            static void apply(auto& observedValues, auto const& childrenRefabricator)
            {
                observedValues.emplaceSideEffects([childrenRefabricator]() -> bool {
                    (*childrenRefabricator)();
                    return false;
                });
                RefabricationSideEffect<Parameters...>::apply(observedValues, childrenRefabricator);
            }
        };
        template <>
        struct RefabricationSideEffect<>
        {
            static void apply(auto&&, auto const&)
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
                    children = std::make_tuple(std::forward<ElementT>(elements)...)](auto& parentElement) {
                auto materialized = parentElement.appendElement(self);
                materialized->appendElements(children);
                return materialized;
            };
        }

        constexpr auto operator()() &&
        {
            return [self = this->clone()](auto& parentElement) {
                auto& materialized = parentElement.appendElement(self);
                return materialized;
            };
        }

        template <typename... ObservedValues, std::invocable... GeneratorT>
        constexpr auto operator()(Reactive<ObservedValues...>&& observedValues, GeneratorT&&... elementGenerators) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    elementGenerators =
                        std::make_tuple(std::forward<GeneratorT>(elementGenerators)...)](auto& parentElement) {
                using ElementType = std::decay_t<decltype(parentElement)>;

                // function is called when observed values change to refabricate the children.
                auto childrenRefabricator = std::make_shared<std::function<void()>>();

                // create the parent for the children, that is the "self" element.
                auto& createdSelf = parentElement.appendElement(self);

                // FIXME: mutable outer then move:
                *childrenRefabricator =
                    [self,
                     observedValues,
                     elementGenerators,
                     createdSelfWeak = std::weak_ptr<ElementType>{createdSelf},
                     childrenRefabricator =
                         std::weak_ptr<typename std::decay_t<decltype(childrenRefabricator)>::element_type>{
                             childrenRefabricator}]() {
                        std::apply(
                            [&observedValues, &childrenRefabricator, &createdSelfWeak](auto&&... elementGenerators) {
                                auto parent = createdSelfWeak.lock();
                                if (!parent)
                                    return;

                                // clear children
                                parent->clearChildren();

                                // regenerate children and (re)register side effect.
                                std::apply(
                                    [&observedValues,
                                     &childrenRefabricator]<typename... GeneratedType>(GeneratedType&&...) mutable {
                                        Detail::RefabricationSideEffect<GeneratedType...>::apply(
                                            observedValues, childrenRefabricator.lock());
                                    },
                                    std::make_tuple(
                                        std::weak_ptr<typename decltype(elementGenerators()(*parent))::element_type>(
                                            elementGenerators()(*parent))...));
                            },
                            elementGenerators);
                    };
                (*childrenRefabricator)();
                return createdSelf;
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