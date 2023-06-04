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

    class HtmlElement;

    namespace Materializers
    {
        /// Creates new actual element and makes it a child of the given parent.
        inline auto appendMaterialize(auto& parent, auto const& htmlElement)
        {
            return parent.appendElement(htmlElement);
        }
        /// Similar to appendMaterialize, but the new element is not added to the children list.
        /// Works together with inplaceMaterialize.
        inline auto fragmentMaterialize(auto& parent, auto const& htmlElement)
        {
            auto elem = parent.makeElement(htmlElement);
            parent.val().template call<Nui::val>("appendChild", elem->val());
            return elem;
        }
        /// Inserts new element at the given position of the given parent.
        inline auto insertMaterialize(std::size_t where, auto& parent, auto const& htmlElement)
        {
            return parent.insert(where, htmlElement);
        }
        /// Replaces the given element with the new one.
        inline auto replaceMaterialize(auto& element, auto const& htmlElement)
        {
            return element.replaceElement(htmlElement);
        }
        /// Used for elements that dont have a direct parent.
        inline auto inplaceMaterialize(auto& element, auto const&)
        {
            return element.template shared_from_base<std::decay_t<decltype(element)>>();
        }
    }

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
                return Materializers::appendMaterialize(element, htmlElement);
            case RendererType::Fragment:
                return Materializers::fragmentMaterialize(element, htmlElement);
            case RendererType::Insert:
                return Materializers::insertMaterialize(gen.metadata, element, htmlElement);
            case RendererType::Replace:
                return Materializers::replaceMaterialize(element, htmlElement);
            case RendererType::Inplace:
                return Materializers::inplaceMaterialize(element, htmlElement);
        }
    };

    //----------------------------------------------------------------------------------------------
    // Workaround Helper Classes for Linkage Bug in Clang 16.
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
        // Children functions:
        template <typename... ElementT>
        requires requires(ElementT&&... elements) {
            std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>{
                std::forward<ElementT>(elements)...};
        }
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>{
                ChildrenRenderer<HtmlElement>{
                    this->clone(),
                    std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>{
                        std::forward<ElementT>(elements)...}}};

            // Unknown Linkage BUG in clang 16 :(
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

            // Unknown Linkage BUG in clang 16 :(
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
        constexpr auto operator()(std::string_view view) &&
        {
            return [self = this->clone(), view](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(view);
                return materialized;
            };
        }
        constexpr auto operator()(char const* text) &&
        {
            return [self = this->clone(), text](auto& parentElement, Renderer const& gen) {
                auto materialized = renderElement(gen, parentElement, self);
                materialized->setTextContent(text);
                return materialized;
            };
        }
        template <typename T>
        requires Fundamental<T>
        constexpr auto operator()(T fundamental) &&
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
            return [self = this->clone(),
                    ElementRenderer = std::forward<GeneratorT>(ElementRenderer)](auto& parentElement, Renderer const&) {
                return ElementRenderer()(parentElement, Renderer{.type = RendererType::Append});
            };
        }
        template <typename T, std::invocable<T&, Renderer const&> GeneratorT>
        requires InvocableReturns<GeneratorT, std::string>
        constexpr auto operator()(GeneratorT&& ElementRenderer) &&
        {
            return [self = this->clone(),
                    ElementRenderer = std::forward<GeneratorT>(ElementRenderer)](auto& parentElement, Renderer const&) {
                return ElementRenderer(parentElement, Renderer{.type = RendererType::Append});
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