#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>

#define NUI_DECLARE_SVG_ELEMENT_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Elements::Svg \
    { \
        struct NAME : HtmlElement \
        { \
            NAME(NAME const&) = default; \
            NAME(NAME&&) = default; \
            NAME(std::vector<Attribute> const& attributes) \
                : HtmlElement{HTML_ACTUAL, &SvgElementBridge, attributes} \
            {} \
            NAME(std::vector<Attribute>&& attributes) \
                : HtmlElement{HTML_ACTUAL, &SvgElementBridge, std::move(attributes)} \
            {} \
            template <typename... T> \
            NAME(T&&... attributes) \
                : HtmlElement{HTML_ACTUAL, &SvgElementBridge, std::forward<T>(attributes)...} \
            {} \
        }; \
    }

#define NUI_DECLARE_SVG_ELEMENT(NAME) NUI_DECLARE_SVG_ELEMENT_RENAME(NAME, #NAME)