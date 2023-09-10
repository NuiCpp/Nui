#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/elements/impl/html_element_bridges.hpp>
#include <nui/frontend/dom/element.hpp>

namespace Nui
{
    template <typename HtmlElem>
    ChildrenRenderer<HtmlElem>::ChildrenRenderer(
        HtmlElem htmlElement,
        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> children)
        : htmlElement_{std::move(htmlElement)}
        , children_{std::move(children)}
    {}

    template <typename HtmlElem>
    std::shared_ptr<Dom::Element>
    ChildrenRenderer<HtmlElem>::operator()(Dom::Element& parentElement, Renderer const& gen) const
    {
        auto materialized = renderElement(gen, parentElement, htmlElement_);
        materialized->appendElements(children_);
        return materialized;
    }

    template <typename HtmlElem>
    TrivialRenderer<HtmlElem>::TrivialRenderer(HtmlElem htmlElement)
        : htmlElement_{std::move(htmlElement)}
    {}

    template <typename HtmlElem>
    std::shared_ptr<Dom::Element>
    TrivialRenderer<HtmlElem>::operator()(Dom::Element& parentElement, Renderer const& gen) const
    {
        return renderElement(gen, parentElement, htmlElement_);
    }
}

#define NUI_DECLARE_HTML_ELEMENT_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Elements \
    { \
        struct NAME : HtmlElement \
        { \
            NAME(NAME const&) = default; \
            NAME(NAME&&) = default; \
            NAME(std::vector<Attribute> const& attributes) \
                : HtmlElement{HTML_ACTUAL, &RegularHtmlElementBridge, attributes} \
            {} \
            NAME(std::vector<Attribute>&& attributes) \
                : HtmlElement{HTML_ACTUAL, &RegularHtmlElementBridge, std::move(attributes)} \
            {} \
            template <typename... T> \
            NAME(T&&... attributes) \
                : HtmlElement{HTML_ACTUAL, &RegularHtmlElementBridge, std::forward<T>(attributes)...} \
            {} \
        }; \
    }

#define NUI_DECLARE_HTML_ELEMENT(NAME) NUI_DECLARE_HTML_ELEMENT_RENAME(NAME, #NAME)