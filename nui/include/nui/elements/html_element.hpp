#pragma once

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/range.hpp>
#include <nui/event_system/event_context.hpp>

#include <tuple>
#include <utility>
#include <concepts>
#include <memory>
#include <functional>
#include <iostream>

namespace Nui
{
    enum class MaterializationStrategy
    {
        Append,
        Replace
    };
    struct GeneratorOptions
    {
        MaterializationStrategy materializationStrategy = MaterializationStrategy::Append;

        auto materialize(auto& element, auto const& htmlElement) const
        {
            if (materializationStrategy == MaterializationStrategy::Append)
                return element.appendElement(htmlElement);
            else
                return element.replaceElement(htmlElement);
        }
    };

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
            return [self = this->clone(), children = std::make_tuple(std::forward<ElementT>(elements)...)](
                       auto& parentElement, GeneratorOptions const& options) {
                auto materialized = options.materialize(parentElement, self);
                materialized->appendElements(children);
                return materialized;
            };
        }

        constexpr auto operator()() &&
        {
            return [self = this->clone()](auto& parentElement, GeneratorOptions const& options) {
                return options.materialize(parentElement, self);
            };
        }

        template <typename... ObservedValues, std::invocable... GeneratorT>
        constexpr auto
        operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&&... elementGenerators) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    elementGenerators = std::make_tuple(std::forward<GeneratorT>(elementGenerators)...)](
                       auto& parentElement, GeneratorOptions const& options) {
                using ElementType = std::decay_t<decltype(parentElement)>;

                // function is called when observed values change to refabricate the children.
                auto childrenRefabricator = std::make_shared<std::function<void()>>();

                // create the parent for the children, that is the "self" element.
                auto&& createdSelf = options.materialize(parentElement, self);

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
                                    [observedValues, childrenRefabricator = childrenRefabricator.lock()](int) -> bool {
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
                    &observedValue = observedRange.observedValue(),
                    elementGenerator = std::forward<GeneratorT>(elementGenerator)](
                       auto& parentElement, GeneratorOptions const& options) {
                using ElementType = std::decay_t<decltype(parentElement)>;
                auto childrenUpdater = std::make_shared<std::function<void()>>();
                auto&& createdSelf = options.materialize(parentElement, self);

                *childrenUpdater = [self,
                                    &observedValue,
                                    elementGenerator,
                                    createdSelfWeak = std::weak_ptr<ElementType>{createdSelf},
                                    childrenUpdater =
                                        std::weak_ptr<typename std::decay_t<decltype(childrenUpdater)>::element_type>{
                                            childrenUpdater}]() mutable {
                    auto parent = createdSelfWeak.lock();
                    if (!parent)
                        return;

                    auto& rangeContext = observedValue.rangeContext();

                    auto updateChildren = [&]() {
                        // Regenerate all elements if necessary:
                        if (rangeContext.isFullRangeUpdate())
                        {
                            std::cout << "Full Range Update\n";
                            parent->clearChildren();
                            for (auto const& element : observedValue.value())
                                elementGenerator(element)(*parent, {MaterializationStrategy::Append});
                            return;
                        }

                        // Update existing elements:
                        std::cout << "---\n";
                        for (auto const& range : rangeContext)
                        {
                            std::cout << '[' << range.low() << ", " << range.high() << "] = " << range.type() << "\n";
                            switch (range.type())
                            {
                                case RangeStateType::Keep:
                                {
                                    continue;
                                }
                                case RangeStateType::Modify:
                                {
                                    for (auto i = range.low(), high = range.high(); i <= high; ++i)
                                    {
                                        elementGenerator(observedValue.value()[i])(
                                            *(*parent)[i], {MaterializationStrategy::Replace});
                                    }
                                    break;
                                }
                                case RangeStateType::Insert:
                                {
                                    break;
                                }
                                default:
                                    break;
                            }
                        }
                    };

                    updateChildren();
                    rangeContext.reset(observedValue.value().size(), false);

                    const auto eventId = globalEventContext.registerEvent(Event{
                        [&observedValue, childrenUpdater = childrenUpdater.lock()](int) -> bool {
                            (*childrenUpdater)();
                            return false;
                        },
                        [createdSelfWeak]() {
                            return !createdSelfWeak.expired();
                        }});
                    observedValue.attachOneshotEvent(eventId);
                };
                (*childrenUpdater)();
                return createdSelf;
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