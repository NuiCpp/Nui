#pragma once

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/range.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/concepts.hpp>

#include <tuple>
#include <utility>
#include <concepts>
#include <memory>
#include <functional>
#include <iostream>

namespace Nui
{
    template <typename DerivedT>
    class GeneratorOptions
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return static_cast<DerivedT const*>(this)->materialize(element, htmlElement);
        }
    };
    class AppendGeneratorOptions : public GeneratorOptions<AppendGeneratorOptions>
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return element.appendElement(htmlElement);
        }
    };
    class InsertGeneratorOptions : public GeneratorOptions<InsertGeneratorOptions>
    {
      public:
        InsertGeneratorOptions(std::size_t where)
            : where_{where}
        {}
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return element.insert(where_, htmlElement);
        }

      private:
        std::size_t where_;
    };
    class ReplaceGeneratorOptions : public GeneratorOptions<ReplaceGeneratorOptions>
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
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

        // Childrem:
        template <typename... ElementT>
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return [self = this->clone(), children = std::make_tuple(std::forward<ElementT>(elements)...)]<typename T>(
                       auto& parentElement, GeneratorOptions<T> const& options) {
                auto materialized = options.materialize(parentElement, self);
                materialized->appendElements(children);
                return materialized;
            };
        }

        // Trivial case:
        constexpr auto operator()() &&
        {
            return [self = this->clone()]<typename T>(auto& parentElement, GeneratorOptions<T> const& options) {
                return options.materialize(parentElement, self);
            };
        }

        // Text content functions:
        constexpr auto operator()(char const* text) &&
        {
            return [self = this->clone(), text]<typename T>(auto& parentElement, GeneratorOptions<T> const& options) {
                auto materialized = options.materialize(parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        auto operator()(std::string text) &&
        {
            return [self = this->clone(),
                    text = std::move(text)]<typename T>(auto& parentElement, GeneratorOptions<T> const& options) {
                auto materialized = options.materialize(parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        constexpr auto operator()(std::string_view view) &&
        {
            return [self = this->clone(), view]<typename T>(auto& parentElement, GeneratorOptions<T> const& options) {
                auto materialized = options.materialize(parentElement, self);
                materialized->setTextContent(view);
                return materialized;
            };
        }
        auto operator()(Observed<std::string>& observedString) &&
        {
            return std::move(*this).operator()(observe(observedString), [&observedString]() -> std::string {
                return observedString.value();
            });
        }
        template <typename GeneratorT>
        requires InvocableReturns<GeneratorT, std::string>
        constexpr auto operator()(GeneratorT&& textGenerator) &&
        {
            return [self = this->clone(), textGenerator = std::forward<GeneratorT>(textGenerator)]<typename T>(
                       auto& parentElement, GeneratorOptions<T> const& options) {
                auto materialized = options.materialize(parentElement, self);
                materialized->setTextContent(textGenerator());
                return materialized;
            };
        }

        // Reactive functions:
        template <typename... ObservedValues, std::invocable... GeneratorT>
        constexpr auto
        operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&&... elementGenerators) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    elementGenerators = std::make_tuple(std::forward<GeneratorT>(elementGenerators)...)]<typename T>(
                       auto& parentElement, GeneratorOptions<T> const& options) {
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
                                if constexpr ((std::is_same_v<decltype(elementGenerators()), std::string> && ...))
                                    (..., parent->setTextContent(elementGenerators()));
                                else
                                    (..., elementGenerators()(*parent, GeneratorOptions<AppendGeneratorOptions>{}));
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
                    elementGenerator = std::forward<GeneratorT>(elementGenerator)]<typename T>(
                       auto& parentElement, GeneratorOptions<T> const& options) {
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
                            parent->clearChildren();
                            for (auto const& element : observedValue.value())
                                elementGenerator(element)(*parent, GeneratorOptions<AppendGeneratorOptions>{});
                            return;
                        }

                        // Insertions:
                        if (const auto insertInterval = rangeContext.insertInterval(); insertInterval)
                        {
                            for (auto i = insertInterval->low(); i <= insertInterval->high(); ++i)
                            {
                                elementGenerator(observedValue.value()[i])(
                                    *parent, InsertGeneratorOptions{static_cast<std::size_t>(i)});
                            }
                            return;
                        }

                        // Update existing elements:
                        for (auto const& range : rangeContext)
                        {
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
                                            *(*parent)[i], ReplaceGeneratorOptions{});
                                    }
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
        }

        std::tuple<Attributes...> const& attributes() const
        {
            return attributes_;
        }

      private:
        std::tuple<Attributes...> attributes_;
    };
}

#define NUI_DECLARE_HTML_ELEMENT(NAME) \
    namespace Nui \
    { \
        struct NAME##_ \
        { \
            constexpr static char const* name = #NAME; \
        }; \
\
        template <typename... Attributes> \
        struct NAME : HtmlElement<NAME##_, Attributes...> \
        { \
            using HtmlElement<NAME##_, Attributes...>::HtmlElement; \
        }; \
        template <typename... Attributes> \
        NAME(Attributes&&...) -> NAME<Attributes...>; \
    }