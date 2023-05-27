#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
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