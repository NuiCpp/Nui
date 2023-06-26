#pragma once

#define NUI_DECLARE_SVG_ELEMENT_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Elements::Svg \
    { \
        struct NAME : HtmlElement \
        { \
            constexpr NAME(NAME const&) = default; \
            constexpr NAME(NAME&&) = default; \
            constexpr NAME(std::vector<Attribute> const& attributes) \
                : HtmlElement{HTML_ACTUAL, &SvgElementBridge, attributes} \
            {} \
            constexpr NAME(std::vector<Attribute>&& attributes) \
                : HtmlElement{HTML_ACTUAL, &SvgElementBridge, std::move(attributes)} \
            {} \
            template <typename... T> \
            constexpr NAME(T&&... attributes) \
                : HtmlElement{HTML_ACTUAL, &SvgElementBridge, std::forward<T>(attributes)...} \
            {} \
        }; \
    }

#define NUI_DECLARE_SVG_ELEMENT(NAME) NUI_DECLARE_SVG_ELEMENT_RENAME(NAME, #NAME)