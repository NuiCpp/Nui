#pragma once

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/event_context.hpp>

#include <tuple>
#include <utility>
#include <concepts>
#include <memory>

#include <iostream>

namespace Nui
{
    namespace Detail
    {
        // class ChildrenRefabricator
        // {
        // public:
        //     void operator()()
        //     {
        //     }

        // private:
        // };
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
        constexpr auto
        operator()(ObservedValueCombinator<ObservedValues...>&& observedValues, GeneratorT&&... elementGenerators) &&
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
                                std::cout << "parent check...\n";

                                auto parent = createdSelfWeak.lock();
                                if (!parent)
                                    return;

                                std::cout << "parent check survived\n";

                                // clear children
                                parent->clearChildren();
                                std::cout << "clear children\n";

                                const auto eventId = globalEventContext.registerEvent(Event{
                                    [childrenRefabricator = childrenRefabricator.lock()]() -> bool {
                                        (*childrenRefabricator)();
                                        return false;
                                    },
                                    [createdSelfWeak]() {
                                        return !createdSelfWeak.expired();
                                    }});

                                //  (re)register side effect.
                                observedValues.attachEvent(eventId);

                                // regenerate children
                                (..., elementGenerators()(*parent));
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