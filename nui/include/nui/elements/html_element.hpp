#pragma once

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/range.hpp>
#include <nui/event_system/event_context.hpp>

#include <tuple>
#include <utility>
#include <concepts>
#include <memory>

namespace Nui
{
    namespace Detail
    {
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
        operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&&... elementGenerators) &&
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
                             childrenRefabricator}]() mutable {
                        std::apply(
                            [&observedValues, &childrenRefabricator, &createdSelfWeak](auto&&... elementGenerators) {
                                auto parent = createdSelfWeak.lock();
                                if (!parent)
                                    return;

                                // clear children
                                parent->clearChildren();

                                const auto eventId = globalEventContext.registerEvent(Event{
                                    [observedValues, childrenRefabricator = childrenRefabricator.lock()](
                                        int, EventImpl::meta_data_type const&) -> bool {
                                        (*childrenRefabricator)();
                                        return false;
                                    },
                                    [createdSelfWeak]() {
                                        return !createdSelfWeak.expired();
                                    }});

                                //  (re)register side effect.
                                observedValues.attachOneshotEvent(eventId);

                                // regenerate children
                                (..., elementGenerators()(*parent));
                            },
                            elementGenerators);
                    };
                (*childrenRefabricator)();
                return createdSelf;
            };
        }

        template <typename ObservedValue, typename GeneratorT>
        constexpr auto operator()(ObservedRange<ObservedValue> observedRange, GeneratorT&& elementGenerator) &&
        {
            return [self = this->clone(),
                    observedRange = std::move(observedRange),
                    elementGenerator = std::forward<GeneratorT>(elementGenerator)](auto& parentElement) {
                using ElementType = std::decay_t<decltype(parentElement)>;
                // auto childrenRefabricator = std::make_shared<std::function<void()>>();
                auto& createdSelf = parentElement.appendElement(self);
            };
            // return this->operator()(ObservedRange{observedValue}, std::forward<GeneratorT>(elementGenerators));
        }

        std::tuple<Attributes...> const& attributes() const
        {
            return attributes_;
        }

      private:
        std::tuple<Attributes...> attributes_;
    };
}