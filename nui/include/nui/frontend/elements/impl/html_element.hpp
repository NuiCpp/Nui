#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/frontend/event_system/range.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/dom/element_fwd.hpp>
#include <nui/frontend/elements/detail/fragment_context.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/concepts.hpp>
#include <nui/utility/scope_exit.hpp>

#include <emscripten/val.h>

#include <vector>
#include <utility>
#include <concepts>
#include <memory>
#include <functional>
#include <optional>
#include <initializer_list>

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

    class HtmlElement
    {
      public:
        friend class DomElement;

        constexpr HtmlElement(HtmlElement const&) = default;
        constexpr HtmlElement(HtmlElement&&) = default;
        virtual ~HtmlElement() = default;
        constexpr HtmlElement(char const* name, std::vector<Attribute> const& attributes)
            : name_{name}
            , attributes_{attributes}
        {}
        constexpr HtmlElement(char const* name, std::vector<Attribute>&& attributes)
            : name_{name}
            , attributes_{std::move(attributes)}
        {}
        template <typename... T>
        constexpr HtmlElement(char const* name, T&&... attributes)
            : name_{name}
            , attributes_{std::forward<T>(attributes)...}
        {}

        HtmlElement clone() const
        {
            return {name_, attributes_};
        }

      private:
        template <typename... ObservedValues, std::invocable GeneratorT>
        constexpr auto
        reactiveRender(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    ElementRenderer =
                        std::forward<GeneratorT>(ElementRenderer)](auto& parentElement, Renderer const& gen) {
                using ElementType = std::decay_t<decltype(parentElement)>;

                // function is called when observed values change to refabricate the children.
                auto childrenRefabricator = std::make_shared<std::function<void()>>();

                auto&& createdSelf = renderElement(gen, parentElement, self);

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

        template <typename ObservedValue, typename GeneratorT>
        constexpr auto rangeRender(ObservedRange<ObservedValue> observedRange, GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(),
                    &observedValue = observedRange.observedValue(),
                    ElementRenderer =
                        std::forward<GeneratorT>(ElementRenderer)](auto& parentElement, Renderer const& gen) {
                if (gen.type == RendererType::Inplace)
                    throw std::runtime_error("fragments are not supported for range generators");

                using ElementType = std::decay_t<decltype(parentElement)>;
                auto childrenUpdater = std::make_shared<std::function<void()>>();
                auto&& createdSelf = renderElement(gen, parentElement, self);

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
                    // TODO: remove fully here:
                    // rangeContext.reset(observedValue.value().size(), false);
                    Detail::createUpdateEvent(observedValue, childrenUpdater, createdSelfWeak);
                };
                (*childrenUpdater)();
                return createdSelf;
            };
        }

      public:
        template <typename... ElementT>
        requires(!IsObserved<ElementT> && ...)
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return
                [self = this->clone(),
                 children = std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>{
                     std::forward<ElementT>(elements)...}](auto& parentElement, Renderer const& gen) {
                    auto materialized = renderElement(gen, parentElement, self);
                    materialized->appendElements(children);
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

        // Text content functions:
        constexpr auto operator()(char const* text) &&
        {
            return [self = this->clone(), text](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
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
        constexpr auto operator()(std::string_view view) &&
        {
            return [self = this->clone(), view](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(view);
                return materialized;
            };
        }
        template <typename T>
        requires Fundamental<T>
        auto operator()(Observed<T> const& observedNumber) &&
        {
            return std::move(*this).operator()(observe(observedNumber), [&observedNumber]() -> std::string {
                return std::to_string(observedNumber.value());
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
        template <std::invocable GeneratorT>
        requires(!InvocableReturns<GeneratorT, std::string>)
        constexpr auto operator()(GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(), ElementRenderer = std::forward<GeneratorT>(ElementRenderer)](
                       auto& parentElement, Renderer const& gen) {
                return ElementRenderer()(parentElement, gen);
            };
        }
        template <typename T, std::invocable<T&, Renderer const&> GeneratorT>
        constexpr auto operator()(GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(), ElementRenderer = std::forward<GeneratorT>(ElementRenderer)](
                       auto& parentElement, Renderer const& gen) {
                return ElementRenderer(parentElement, gen);
            };
        }

        // Reactive functions:
        template <typename... ObservedValues, std::invocable GeneratorT>
        constexpr auto operator()(ObservedValueCombinatorWithGenerator<GeneratorT, ObservedValues...> combinator) &&
        {
            return std::move(*this).operator()(std::move(combinator).split(), std::move(combinator).generator());
        }
        template <typename... ObservedValues, std::invocable GeneratorT>
        constexpr auto
        operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&& ElementRenderer) &&
        {
            return std::move(*this).reactiveRender(
                std::move(observedValues), std::forward<GeneratorT>(ElementRenderer));
        }
        template <typename ObservedValue, typename GeneratorT>
        constexpr auto operator()(ObservedRange<ObservedValue> observedRange, GeneratorT&& ElementRenderer) &&
        {
            return std::move(*this).rangeRender(std::move(observedRange), std::forward<GeneratorT>(ElementRenderer));
        }
        template <typename ObservedValue, typename GeneratorT>
        constexpr auto operator()(std::pair<ObservedRange<ObservedValue>, GeneratorT>&& mapPair) &&
        {
            return std::move(*this).rangeRender(std::move(mapPair.first), std::move(mapPair.second));
        }
        auto operator()(Observed<std::string> const& observedString) &&
        {
            return std::move(*this).operator()(observe(observedString), [&observedString]() -> std::string {
                return observedString.value();
            });
        }

        std::vector<Attribute> const& attributes() const
        {
            return attributes_;
        }

        char const* name() const
        {
            return name_;
        }

      private:
        char const* name_;
        std::vector<Attribute> attributes_;
    };
}

#define NUI_DECLARE_HTML_ELEMENT_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Elements \
    { \
        struct NAME : HtmlElement \
        { \
            constexpr NAME(NAME const&) = default; \
            constexpr NAME(NAME&&) = default; \
            constexpr NAME(std::vector<Attribute> const& attributes) \
                : HtmlElement{HTML_ACTUAL, attributes} \
            {} \
            constexpr NAME(std::vector<Attribute>&& attributes) \
                : HtmlElement{HTML_ACTUAL, std::move(attributes)} \
            {} \
            template <typename... T> \
            constexpr NAME(T&&... attributes) \
                : HtmlElement{HTML_ACTUAL, std::forward<T>(attributes)...} \
            {} \
        }; \
    }

#define NUI_DECLARE_HTML_ELEMENT(NAME) NUI_DECLARE_HTML_ELEMENT_RENAME(NAME, #NAME)