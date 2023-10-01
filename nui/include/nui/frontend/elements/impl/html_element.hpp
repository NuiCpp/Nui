#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/frontend/event_system/range.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/dom/element_fwd.hpp>
#include <nui/frontend/elements/detail/fragment_context.hpp>
#include <nui/frontend/elements/impl/html_element_bridge.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/elements/impl/materialize.hpp>
#include <nui/frontend/elements/impl/make_children_update_event.hpp>
#include <nui/frontend/elements/impl/range_renderer.hpp>
#include <nui/concepts.hpp>
#include <nui/utility/scope_exit.hpp>

#include <nui/frontend/val.hpp>

#include <vector>
#include <utility>
#include <concepts>
#include <memory>
#include <functional>
#include <optional>
#include <initializer_list>

namespace Nui
{
    class HtmlElement;

    //----------------------------------------------------------------------------------------------
    // Workaround Helper Classes for Linkage Bug in WASM-LD.
    //----------------------------------------------------------------------------------------------
    template <typename HtmlElem>
    struct ChildrenRenderer
    {
      public:
        ChildrenRenderer(
            HtmlElem htmlElement,
            std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> children);

        std::shared_ptr<Dom::Element> operator()(Dom::Element& parentElement, Renderer const& gen) const;

      private:
        HtmlElem htmlElement_;
        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> children_;
    };
    template <typename HtmlElem>
    struct TrivialRenderer
    {
      public:
        TrivialRenderer(HtmlElem htmlElement);
        std::shared_ptr<Dom::Element> operator()(Dom::Element& parentElement, Renderer const& gen) const;

      private:
        HtmlElem htmlElement_;
    };
    //----------------------------------------------------------------------------------------------
    class HtmlElement
    {
      public:
        friend class DomElement;

        HtmlElement(HtmlElement const&) = default;
        HtmlElement(HtmlElement&&) = default;
        virtual ~HtmlElement() = default;
        HtmlElement(char const* name, HtmlElementBridge const* bridge, std::vector<Attribute> const& attributes)
            : name_{name}
            , bridge_{bridge}
            , attributes_{attributes}
        {}
        HtmlElement(char const* name, HtmlElementBridge const* bridge, std::vector<Attribute>&& attributes)
            : name_{name}
            , bridge_{bridge}
            , attributes_{std::move(attributes)}
        {}
        template <typename... T>
        HtmlElement(char const* name, HtmlElementBridge const* bridge, T&&... attributes)
            : name_{name}
            , bridge_{bridge}
            , attributes_{std::forward<T>(attributes)...}
        {}

        HtmlElement clone() const
        {
            return {name_, bridge_, attributes_};
        }

      private:
        template <typename... ObservedValues, std::invocable GeneratorT>
        auto reactiveRender(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&& elementRenderer) &&
        {
            return [self = this->clone(),
                    observedValues = std::move(observedValues),
                    elementRenderer =
                        std::forward<GeneratorT>(elementRenderer)](auto& parentElement, Renderer const& gen) {
                using ElementType = std::decay_t<decltype(parentElement)>;

                // function is called when observed values change to refabricate the children.
                auto childrenRefabricator = std::make_shared<std::function<void()>>();

                auto&& createdSelf = renderElement(gen, parentElement, self);

                if (gen.type == RendererType::Inplace)
                {
                    *childrenRefabricator = [observedValues,
                                             elementRenderer,
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

                        Detail::makeChildrenUpdateEvent(observedValues, childrenRefabricator, createdSelfWeak);

                        // regenerate children
                        if constexpr ((std::is_same_v<decltype(elementRenderer()), std::string>))
                            parent->setTextContent(elementRenderer());
                        else
                            fragmentContext.push(elementRenderer()(*parent, Renderer{.type = RendererType::Fragment}));
                    };
                }
                else
                {
                    *childrenRefabricator = [observedValues,
                                             elementRenderer,
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

                        Detail::makeChildrenUpdateEvent(observedValues, childrenRefabricator, createdSelfWeak);

                        // regenerate children
                        if constexpr ((std::is_same_v<decltype(elementRenderer()), std::string>))
                            parent->setTextContent(elementRenderer());
                        else
                            elementRenderer()(*parent, Renderer{.type = RendererType::Append});
                    };
                }

                (*childrenRefabricator)();
                return createdSelf;
            };
        }

        template <typename RangeType, typename GeneratorT>
        auto rangeRender(RangeType&& valueRange, GeneratorT&& elementRenderer) &&
        {
            return [self = this->clone(),
                    rangeRenderer =
                        std::make_shared<Detail::RangeRenderer<RangeType, GeneratorT, RangeType::isRandomAccess>>(
                            std::move(valueRange).underlying(), std::forward<GeneratorT>(elementRenderer))](
                       auto& parentElement, Renderer const& gen) {
                if (gen.type == RendererType::Inplace)
                    throw std::runtime_error("fragments are not supported for range generators");

                auto&& materialized = renderElement(gen, parentElement, self);
                (*rangeRenderer)(materialized);
                return materialized;
            };
        }

      public:
        // Children functions:
        template <typename... ElementT>
        requires requires(ElementT&&... elements) {
            std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>{
                std::forward<ElementT>(elements)...};
        }
        auto operator()(ElementT&&... elements) &&
        {
            return std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>{
                ChildrenRenderer<HtmlElement>{
                    this->clone(),
                    std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>{
                        std::forward<ElementT>(elements)...}}};

            // Unknown Linkage BUG in wasm-ld :(
            // return
            //     [self = this->clone(),
            //      children = std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer
            //      const&)>>{
            //          std::forward<ElementT>(elements)...}](auto& parentElement, Renderer const& gen) {
            //         auto materialized = renderElement(gen, parentElement, self);
            //         materialized->appendElements(children);
            //         return materialized;
            //     };
        }

        // Trivial case:
        auto operator()() &&
        {
            return std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>{
                TrivialRenderer<HtmlElement>{this->clone()}};

            // Unknown Linkage BUG in wasm-ld :(
            // return [self = this->clone()](auto& parentElement, Renderer const& gen) {
            //     return renderElement(gen, parentElement, self);
            // };
        }

        // Text content functions:
        template <typename T>
        requires std::same_as<std::decay_t<T>, std::string>
        auto operator()(T&& text) &&
        {
            return [self = this->clone(), text = std::forward<T>(text)](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        auto operator()(std::string_view view) &&
        {
            return [self = this->clone(), view](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(view);
                return materialized;
            };
        }
        auto operator()(char const* text) &&
        {
            return [self = this->clone(), text](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        template <typename T>
        requires Fundamental<T>
        auto operator()(T fundamental) &&
        {
            return [self = this->clone(), fundamental](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(std::to_string(fundamental));
                return materialized;
            };
        }

        // Generator functions:
        template <typename GeneratorT>
        requires InvocableReturns<GeneratorT, std::string>
        auto operator()(GeneratorT&& textGenerator) &&
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
        auto operator()(GeneratorT&& elementRenderer) &&
        {
            return [self = this->clone(),
                    elementRenderer = std::forward<GeneratorT>(elementRenderer)](auto& parentElement, Renderer const&) {
                return elementRenderer()(parentElement, Renderer{.type = RendererType::Append});
            };
        }
        template <typename T, std::invocable<T&, Renderer const&> GeneratorT>
        requires InvocableReturns<GeneratorT, std::string>
        auto operator()(GeneratorT&& elementRenderer) &&
        {
            return [self = this->clone(),
                    elementRenderer = std::forward<GeneratorT>(elementRenderer)](auto& parentElement, Renderer const&) {
                return elementRenderer(parentElement, Renderer{.type = RendererType::Append});
            };
        }

        // Reactive functions:
        template <typename... ObservedValues, std::invocable GeneratorT>
        auto operator()(ObservedValueCombinatorWithGenerator<GeneratorT, ObservedValues...> combinator) &&
        {
            return std::move(*this).operator()(std::move(combinator).split(), std::move(combinator).generator());
        }
        template <typename... ObservedValues, std::invocable GeneratorT>
        auto operator()(ObservedValueCombinator<ObservedValues...> observedValues, GeneratorT&& elementRenderer) &&
        {
            return std::move(*this).reactiveRender(
                std::move(observedValues), std::forward<GeneratorT>(elementRenderer));
        }

        // Range functions:
        template <typename ObservedValue, typename GeneratorT>
        auto operator()(ObservedRange<ObservedValue> observedRange, GeneratorT&& elementRenderer) &&
        {
            return std::move(*this).rangeRender(std::move(observedRange), std::forward<GeneratorT>(elementRenderer));
        }
        template <typename ObservedValue, typename GeneratorT>
        auto operator()(std::pair<ObservedRange<ObservedValue>, GeneratorT>&& mapPair) &&
        {
            return std::move(*this).rangeRender(std::move(mapPair.first), std::move(mapPair.second));
        }
        template <typename IteratorT, typename GeneratorT, typename... ObservedT>
        auto operator()(UnoptimizedRange<IteratorT, ObservedT...>&& unoptimizedRange, GeneratorT&& elementRenderer) &&
        {
            return [self = this->clone(),
                    rangeRenderer =
                        std::make_shared<Detail::UnoptimizedRangeRenderer<IteratorT, GeneratorT, ObservedT...>>(
                            std::move(unoptimizedRange), std::forward<GeneratorT>(elementRenderer))](
                       auto& parentElement, Renderer const& gen) {
                if (gen.type == RendererType::Inplace)
                    throw std::runtime_error("fragments are not supported for range generators");

                auto&& materialized = renderElement(gen, parentElement, self);
                (*rangeRenderer)(materialized);
                return materialized;
            };
        }

        // Observed text and number content functions:
        inline auto operator()(Observed<std::string> const& observedString) &&
        {
            return std::move(*this).operator()(observe(observedString), [&observedString]() -> std::string {
                return observedString.value();
            });
        }
        template <typename T>
        requires Fundamental<T>
        auto operator()(Observed<T> const& observedNumber) &&
        {
            return std::move(*this).operator()(observe(observedNumber), [&observedNumber]() -> std::string {
                return std::to_string(observedNumber.value());
            });
        }

        inline std::vector<Attribute> const& attributes() const
        {
            return attributes_;
        }

        inline char const* name() const
        {
            return name_;
        }

        inline HtmlElementBridge const* bridge() const
        {
            return bridge_;
        }

      private:
        char const* name_;
        HtmlElementBridge const* bridge_;
        std::vector<Attribute> attributes_;
    };
}