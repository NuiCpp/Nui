#pragma once

#include <nui/frontend/elements/impl/html_element_incl.hpp>

namespace Nui::Elements
{
    struct text : HtmlElement
    {
        HTML_ELEMENT_CONSTEXPR text(text const&) = default;
        HTML_ELEMENT_CONSTEXPR text(text&&) = default;
        HTML_ELEMENT_CONSTEXPR text(std::string_view content)
            : HtmlElement{"", &TextElementBridge, Attribute{content}}
        {}
    };
}