#pragma once

#include <nui/elements/html_element.hpp>

namespace Nui
{
    struct button_
    {
        constexpr static char const* name = "button";
    };

    template <typename... Attributes>
    struct button : HtmlElement<button_, Attributes...>
    {
        using HtmlElement<button_, Attributes...>::HtmlElement;
    };
    template <typename... Attributes>
    button(Attributes&&...) -> button<Attributes...>;
}