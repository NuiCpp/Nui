#pragma once

#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/event_system/range.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/concepts.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/dom/reference.hpp>
#include <nui/elements/detail/fragment_context.hpp>

#include <emscripten/val.h>

#include <tuple>
#include <utility>
#include <concepts>
#include <memory>
#include <functional>
#include <iostream>
#include <optional>

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
    class RendererOptions
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return static_cast<DerivedT const*>(this)->materialize(element, htmlElement);
        }
    };
    class AppendRendererOptions : public RendererOptions<AppendRendererOptions>
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return element.appendElement(htmlElement);
        }
    };
    class FragmentRendererOptions : public RendererOptions<FragmentRendererOptions>
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            auto elem = element.makeElement(htmlElement);
            element.val().template call<emscripten::val>("appendChild", elem->val());
            return elem;
        }
    };
    class InsertRendererOptions : public RendererOptions<InsertRendererOptions>
    {
      public:
        InsertRendererOptions(std::size_t where)
            : where_{where}
        {}
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return element.insert(where_, htmlElement);
        }

      private:
        std::size_t where_;
    };
    class ReplaceRendererOptions : public RendererOptions<ReplaceRendererOptions>
    {
      public:
        auto materialize(auto& element, auto const& htmlElement) const
        {
            return element.replaceElement(htmlElement);
        }
    };
    class InplaceRendererOptions : public RendererOptions<InplaceRendererOptions>
    {
      public:
        auto materialize(auto& element, auto const&) const
        {
            return element.template shared_from_base<std::decay_t<decltype(element)>>();
        }
    };
    enum class RendererType
    {
        Append,
        Fragment,
        Insert,
        Replace,
        Inplace
    };
    struct Renderer
    {
        RendererType type;
        std::size_t metadata;
    };
    auto renderElement(Renderer const& gen, auto& element, auto const& htmlElement)
    {
        switch (gen.type)
        {
            case RendererType::Append:
                return AppendRendererOptions{}.materialize(element, htmlElement);
            case RendererType::Fragment:
                return FragmentRendererOptions{}.materialize(element, htmlElement);
            case RendererType::Insert:
                return InsertRendererOptions{gen.metadata}.materialize(element, htmlElement);
            case RendererType::Replace:
                return ReplaceRendererOptions{}.materialize(element, htmlElement);
            case RendererType::Inplace:
                return InplaceRendererOptions{}.materialize(element, htmlElement);
        }
    };

    template <typename Derived, typename... Attributes>
    class HtmlElement
    {
      public:
        friend class DomElement;

        constexpr HtmlElement(HtmlElement const&) = default;
        constexpr HtmlElement(HtmlElement&&) = default;
        constexpr HtmlElement(std::tuple<Attributes...> const& attributes)
            : attributes_{attributes}
        {}
        constexpr HtmlElement(std::tuple<Attributes...>&& attributes)
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
        requires(Dom::IsNotReferencePasser<ElementT>&&...) constexpr auto operator()(ElementT&&... elements) &&
        {
            return [self = this->clone(), children = std::make_tuple(std::forward<ElementT>(elements)...)](
                       auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->appendElements(children);
                return materialized;
            };
        }
        template <typename ReferencePasserT, typename... ElementT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        constexpr auto operator()(ReferencePasserT&& referencePasser, ElementT&&... elements) &&
        {
            return [self = this->clone(),
                    referencePasser = std::forward<ReferencePasserT>(referencePasser),
                    children = std::make_tuple(std::forward<ElementT>(elements)...)](
                       auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->appendElements(children);
                referencePasser(materialized);
                return materialized;
            };
        }

        // Trivial case:
        constexpr auto operator()() &&
        {
            return [self = this->clone()](auto& parentElement, Renderer const& gen) {
                return renderElement(gen, parentElement, self);
            };
        }
        template <typename ReferencePasserT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        constexpr auto operator()(ReferencePasserT&& referencePasser)
        {
            return [self = this->clone(), referencePasser = std::forward<ReferencePasserT>(referencePasser)](
                       auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                referencePasser(materialized);
                return materialized;
            };
        }

        // Text content functions:
        constexpr auto operator()(char const* text) &&
        {
            return [self = this->clone(), text](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        template <typename ReferencePasserT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        constexpr auto operator()(ReferencePasserT&& referencePasser, char const* text) &&
        {
            return [self = this->clone(), referencePasser = std::forward<ReferencePasserT>(referencePasser), text](
                       auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                referencePasser(materialized);
                return materialized;
            };
        }
        auto operator()(std::string text) &&
        {
            return [self = this->clone(), text = std::move(text)](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        template <typename ReferencePasserT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        auto operator()(ReferencePasserT&& referencePasser, std::string text) &&
        {
            return [self = this->clone(),
                    referencePasser = std::forward<ReferencePasserT>(referencePasser),
                    text = std::move(text)](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                referencePasser(materialized);
                return materialized;
            };
        }
        constexpr auto operator()(std::string_view view) &&
        {
            return [self = this->clone(), view](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(view);
                return materialized;
            };
        }
        template <typename ReferencePasserT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        constexpr auto operator()(ReferencePasserT&& referencePasser, std::string_view view) &&
        {
            return [self = this->clone(), referencePasser = std::forward<ReferencePasserT>(referencePasser), view](
                       auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(view);
                referencePasser(materialized);
                return materialized;
            };
        }
        auto operator()(Observed<std::string>& observedString) &&
        {
            return std::move(*this).operator()(observe(observedString), [&observedString]() -> std::string {
                return observedString.value();
            });
        }
        template <typename ReferencePasserT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        auto operator()(ReferencePasserT&& referencePasser, Observed<std::string>& observedString) &&
        {
            return std::move(*this).operator()(
                std::forward<ReferencePasserT>(referencePasser),
                observe(observedString),
                [&observedString]() -> std::string {
                    return observedString.value();
                });
        }
        template <typename GeneratorT>
        requires InvocableReturns<GeneratorT, std::string>
        constexpr auto operator()(GeneratorT&& textGenerator) &&
        {
            return [self = this->clone(),
                    textGenerator = std::forward<GeneratorT>(textGenerator)](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(textGenerator());
                return materialized;
            };
        }
        template <typename ReferencePasserT, typename GeneratorT>
        requires Dom::IsReferencePasser<ReferencePasserT> && InvocableReturns<GeneratorT, std::string>
        constexpr auto operator()(ReferencePasserT&& referencePasser, GeneratorT&& textGenerator) &&
        {
            return [self = this->clone(),
                    referencePasser = std::forward<ReferencePasserT>(referencePasser),
                    textGenerator = std::forward<GeneratorT>(textGenerator)](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(textGenerator());
                referencePasser(materialized);
                return materialized;
            };
        }
        template <std::invocable GeneratorT>
        constexpr auto operator()(GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(), ElementRenderer = std::forward<GeneratorT>(ElementRenderer)](
                       auto& parentElement, Renderer const& gen) {
                return ElementRenderer()(parentElement, gen);
            };
        }

        // Reactive functions:
        template <typename ReferencePasserT, typename... ObservedValues, std::invocable GeneratorT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        constexpr auto operator()(
            ReferencePasserT&& referencePasser,
            ObservedValueCombinator<ObservedValues...> observedValues,
            GeneratorT&& ElementRenderer) &&
        {
            return std::move(*this).reactiveRender(
                std::forward<ReferencePasserT>(referencePasser),
                std::move(observedValues),
                std::forward<GeneratorT>(ElementRenderer));
        }
        template <typename... ObservedValues, std::invocable GeneratorT>
        constexpr auto
        operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&& ElementRenderer) &&
        {
            return std::move(*this).reactiveRender(
                Dom::ReferencePasser{[](auto&&) {}},
                std::move(observedValues),
                std::forward<GeneratorT>(ElementRenderer));
        }

        template <typename ReferencePasserT, typename ObservedValue, typename GeneratorT>
        constexpr auto operator()(
            ReferencePasserT&& referencePasser,
            ObservedRange<ObservedValue> observedRange,
            GeneratorT&& ElementRenderer) &&
        {
            return std::move(*this).rangeRender(
                std::forward<ReferencePasserT>(referencePasser),
                std::move(observedRange),
                std::forward<GeneratorT>(ElementRenderer));
        }
        template <typename ObservedValue, typename GeneratorT>
        constexpr auto operator()(ObservedRange<ObservedValue> observedRange, GeneratorT&& ElementRenderer) &&
        {
            return std::move(*this).rangeRender(
                Dom::ReferencePasser{[](auto&&) {}},
                std::move(observedRange),
                std::forward<GeneratorT>(ElementRenderer));
        }

        std::tuple<Attributes...> const& attributes() const
        {
            return attributes_;
        }

      private:
        template <typename ReferencePasserT, typename... ObservedValues, std::invocable GeneratorT>
        requires Dom::IsReferencePasser<ReferencePasserT>
        constexpr auto reactiveRender(
            ReferencePasserT&& referencePasser,
            ObservedValueCombinator<ObservedValues...> observedValues,
            GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(),
                    referencePasser = std::forward<ReferencePasserT>(referencePasser),
                    observedValues = std::move(observedValues),
                    ElementRenderer =
                        std::forward<GeneratorT>(ElementRenderer)](auto& parentElement, Renderer const& gen) {
                using ElementType = std::decay_t<decltype(parentElement)>;

                // function is called when observed values change to refabricate the children.
                auto childrenRefabricator = std::make_shared<std::function<void()>>();

                auto&& createdSelf = renderElement(gen, parentElement, self);
                referencePasser(createdSelf);

                if (gen.type == RendererType::Inplace)
                {
                    *childrenRefabricator = [observedValues,
                                             ElementRenderer,
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
                        if constexpr ((std::is_same_v<decltype(ElementRenderer()), std::string>))
                            parent->setTextContent(ElementRenderer());
                        else
                            fragmentContext.push(ElementRenderer()(*parent, Renderer{.type = RendererType::Fragment}));
                    };
                }
                else
                {
                    *childrenRefabricator = [observedValues,
                                             ElementRenderer,
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
                        if constexpr ((std::is_same_v<decltype(ElementRenderer()), std::string>))
                            parent->setTextContent(ElementRenderer());
                        else
                            ElementRenderer()(*parent, Renderer{.type = RendererType::Append});
                    };
                }

                (*childrenRefabricator)();
                return createdSelf;
            };
        }

        template <typename ReferencePasserT, typename ObservedValue, typename GeneratorT>
        constexpr auto rangeRender(
            ReferencePasserT&& referencePasser,
            ObservedRange<ObservedValue> observedRange,
            GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(),
                    referencePasser = std::forward<ReferencePasserT>(referencePasser),
                    &observedValue = observedRange.observedValue(),
                    ElementRenderer =
                        std::forward<GeneratorT>(ElementRenderer)](auto& parentElement, Renderer const& gen) {
                if (gen.type == RendererType::Inplace)
                    throw std::runtime_error("fragments are not supported for range generators");

                using ElementType = std::decay_t<decltype(parentElement)>;
                auto childrenUpdater = std::make_shared<std::function<void()>>();
                auto&& createdSelf = renderElement(gen, parentElement, self);
                referencePasser(createdSelf);

                *childrenUpdater = [&observedValue,
                                    ElementRenderer,
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
                                ElementRenderer(counter++, element)(*parent, Renderer{.type = RendererType::Append});
                            return;
                        }

                        // Insertions:
                        if (const auto insertInterval = rangeContext.insertInterval(); insertInterval)
                        {
                            if constexpr (ObservedValue::isRandomAccess)
                            {
                                for (auto i = insertInterval->low(); i <= insertInterval->high(); ++i)
                                {
                                    ElementRenderer(i, observedValue.value()[i])(
                                        *parent,
                                        Renderer{
                                            .type = RendererType::Insert, .metadata = static_cast<std::size_t>(i)});
                                }
                            }
                            else
                            {
                                // There is no optimization enabled for non random access containers
                                return;
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
                                    if constexpr (ObservedValue::isRandomAccess)
                                    {
                                        for (auto i = range.low(), high = range.high(); i <= high; ++i)
                                        {
                                            ElementRenderer(i, observedValue.value()[i])(
                                                *(*parent)[i], Renderer{.type = RendererType::Replace});
                                        }
                                    }
                                    else
                                    {
                                        // There is no optimization enabled for non random access containers
                                        return;
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

      private:
        std::tuple<Attributes...> attributes_;
    };
}

#define NUI_DECLARE_HTML_ELEMENT_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui \
    { \
        struct NAME##_ \
        { \
            constexpr static char const* name = #HTML_ACTUAL; \
        }; \
\
        template <typename... Attributes> \
        struct NAME : HtmlElement<NAME##_, Attributes...> \
        { \
            using HtmlElement<NAME##_, Attributes...>::HtmlElement; \
        }; \
        template <typename... Attributes> \
        NAME(Attributes&&...) -> NAME<Attributes...>; \
        template <typename... Attributes> \
        NAME(std::tuple<Attributes...>) -> NAME<Attributes...>; \
    }

#define NUI_DECLARE_HTML_ELEMENT(NAME) NUI_DECLARE_HTML_ELEMENT_RENAME(NAME, NAME)