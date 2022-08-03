#pragma once

#include <nui/elements/html_element.hpp>

namespace Nui
{
    struct div_
    {
        constexpr static char const* name = "div";
    };

    template <typename... Attributes>
    struct div : HtmlElement<div_, Attributes...>
    {
        constexpr div(Attributes&&... attr)
            : HtmlElement<div_, Attributes...>{std::move(attr)...}
        {}
    };
}