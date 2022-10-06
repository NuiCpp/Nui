#pragma once

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/range.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/concepts.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/elements/detail/fragment_context.hpp>

#include <emscripten/val.h>

#include <tuple>
#include <utility>
#include <concepts>
#include <memory>
#include <functional>
#include <iostream>

namespace Nui
{
    namespace Detail
    {
        inline void createUpdateEvent(auto& observedValues, auto& childrenRefabricator, auto& createdSelfWeak)
        {
            const auto eventId = globalEventContext.registerEvent(Event{
                [childrenRefabricator](int) -> bool {
                    (*childrenRefabricator)();
                    return false;
                },
                [createdSelfWeak]() {
                    return !createdSelfWeak.expired();
                }});
            observedValues.attachOneshotEvent(eventId);
        }
    }

    // TODO: refactor, not anymore needed as classes:
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
    class FragmentGeneratorOptions : public GeneratorOptions<FragmentGeneratorOptions>
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            auto elem = element.makeElement(htmlElement);
            element.val().template call<emscripten::val>("appendChild", elem->val());
            return elem;
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
    class InplaceGeneratorOptions : public GeneratorOptions<InplaceGeneratorOptions>
    {
      public:
        auto materialize(auto& element, auto const&) const
        {
            return element.template shared_from_base<std::decay_t<decltype(element)>>();
        }
    };
    enum class GeneratorType
    {
        Append,
        Fragment,
        Insert,
        Replace,
        Inplace
    };
    struct Generator
    {
        GeneratorType type;
        std::size_t metadata;
    };
    auto generateElement(Generator const& gen, auto& element, auto const& htmlElement)
    {
        switch (gen.type)
        {
            case GeneratorType::Append:
                return AppendGeneratorOptions{}.materialize(element, htmlElement);
            case GeneratorType::Fragment:
                return FragmentGeneratorOptions{}.materialize(element, htmlElement);
            case GeneratorType::Insert:
                return InsertGeneratorOptions{gen.metadata}.materialize(element, htmlElement);
            case GeneratorType::Replace:
                return ReplaceGeneratorOptions{}.materialize(element, htmlElement);
            case GeneratorType::Inplace:
                return InplaceGeneratorOptions{}.materialize(element, htmlElement);
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

        // Children:
        template <typename... ElementT>
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return [self = this->clone(), children = std::make_tuple(std::forward<ElementT>(elements)...)](
                       auto& parentElement, Generator const& gen) {
                auto materialized = generateElement(gen, parentElement, self);
                materialized->appendElements(children);
                return materialized;
            };
        }

        // Trivial case:
        constexpr auto operator()() &&
        {
            return [self = this->clone()](auto& parentElement, Generator const& gen) {
                return generateElement(gen, parentElement, self);
            };
        }

        // Text content functions:
        constexpr auto operator()(char const* text) &&
        {
            return [self = this->clone(), text](auto& parentElement, Generator const& gen) {
                auto materialized = generateElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        auto operator()(std::string text) &&
        {
            return [self = this->clone(), text = std::move(text)](auto& parentElement, Generator const& gen) {
                auto materialized = generateElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        constexpr auto operator()(std::string_view view) &&
        {
            return [self = this->clone(), view](auto& parentElement, Generator const& gen) {
                auto materialized = generateElement(gen, parentElement, self);
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
            return [self = this->clone(), textGenerator = std::forward<GeneratorT>(textGenerator)](
                       auto& parentElement, Generator const& gen) {
                auto materialized = generateElement(gen, parentElement, self);
                materialized->setTextContent(textGenerator());
                return materialized;
            };
        }
        template <std::invocable GeneratorT>
        constexpr auto operator()(GeneratorT&& elementGenerator) &&
        {
            return [self = this->clone(), elementGenerator = std::forward<GeneratorT>(elementGenerator)](
                       auto& parentElement, Generator const& gen) {
                return elementGenerator()(parentElement, gen);
            };
        }

        // Reactive functions:
        template <typename... ObservedValues, std::invocable GeneratorT>
        constexpr auto
        operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&& elementGenerator) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    elementGenerator =
                        std::forward<GeneratorT>(elementGenerator)](auto& parentElement, Generator const& gen) {
                using ElementType = std::decay_t<decltype(parentElement)>;

                // function is called when observed values change to refabricate the children.
                auto childrenRefabricator = std::make_shared<std::function<void()>>();

                auto&& createdSelf = generateElement(gen, parentElement, self);

                if (gen.type == GeneratorType::Inplace)
                {
                    *childrenRefabricator = [self,
                                             observedValues,
                                             elementGenerator,
                                             fragmentContext = Detail::FragmentContext<ElementType>{},
                                             createdSelfWeak = std::weak_ptr<ElementType>(createdSelf),
                                             childrenRefabricator]() mutable {
                        fragmentContext.clear();

                        auto parent = createdSelfWeak.lock();
                        if (!parent)
                        {
                            childrenRefabricator.reset();
                            return;
                        }

                        Detail::createUpdateEvent(observedValues, childrenRefabricator, createdSelfWeak);

                        // regenerate children
                        if constexpr ((std::is_same_v<decltype(elementGenerator()), std::string>))
                            parent->setTextContent(elementGenerator());
                        else
                            fragmentContext.push(
                                elementGenerator()(*parent, Generator{.type = GeneratorType::Fragment}));
                    };
                }
                else
                {
                    *childrenRefabricator = [self,
                                             observedValues,
                                             elementGenerator,
                                             createdSelfWeak = std::weak_ptr<ElementType>(createdSelf),
                                             childrenRefabricator]() mutable {
                        auto parent = createdSelfWeak.lock();
                        if (!parent)
                        {
                            childrenRefabricator.reset();
                            return;
                        }

                        // clear children
                        parent->clearChildren();

                        Detail::createUpdateEvent(observedValues, childrenRefabricator, createdSelfWeak);

                        // regenerate children
                        if constexpr ((std::is_same_v<decltype(elementGenerator()), std::string>))
                            parent->setTextContent(elementGenerator());
                        else
                            elementGenerator()(*parent, Generator{.type = GeneratorType::Append});
                    };
                }

                (*childrenRefabricator)();
                return createdSelf;
            };
        }
        template <typename ObservedValue, typename GeneratorT>
        constexpr auto operator()(ObservedRange<ObservedValue> observedRange, GeneratorT&& elementGenerator) &&
        {
            return [self = this->clone(),
                    &observedValue = observedRange.observedValue(),
                    elementGenerator =
                        std::forward<GeneratorT>(elementGenerator)](auto& parentElement, Generator const& gen) {
                if (gen.type == GeneratorType::Inplace)
                    throw std::runtime_error("fragments are not supported for range generators");

                using ElementType = std::decay_t<decltype(parentElement)>;
                auto childrenUpdater = std::make_shared<std::function<void()>>();
                auto&& createdSelf = generateElement(gen, parentElement, self);

                *childrenUpdater = [self,
                                    &observedValue,
                                    elementGenerator,
                                    createdSelfWeak = std::weak_ptr<ElementType>(createdSelf),
                                    childrenUpdater]() mutable {
                    auto parent = createdSelfWeak.lock();
                    if (!parent)
                    {
                        childrenUpdater.reset();
                        return;
                    }

                    auto& rangeContext = observedValue.rangeContext();
                    auto updateChildren = [&]() {
                        // Regenerate all elements if necessary:
                        if (rangeContext.isFullRangeUpdate())
                        {
                            parent->clearChildren();
                            long counter = 0;
                            for (auto const& element : observedValue.value())
                                elementGenerator(++counter, element)(*parent, Generator{.type = GeneratorType::Append});
                            return;
                        }

                        // Insertions:
                        if (const auto insertInterval = rangeContext.insertInterval(); insertInterval)
                        {
                            for (auto i = insertInterval->low(); i <= insertInterval->high(); ++i)
                            {
                                elementGenerator(i, observedValue.value()[i])(
                                    *parent,
                                    Generator{.type = GeneratorType::Insert, .metadata = static_cast<std::size_t>(i)});
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
                                        elementGenerator(i, observedValue.value()[i])(
                                            *(*parent)[i], Generator{.type = GeneratorType::Replace});
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
                    Detail::createUpdateEvent(observedValue, childrenUpdater, createdSelfWeak);
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