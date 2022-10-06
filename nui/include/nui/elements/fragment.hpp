#pragma once

#include <nui/elements/html_element.hpp>

#include <tuple>

namespace Nui
{
    namespace Detail
    {
        struct fragment_
        {
            constexpr static char const* name = "fragmenterror";
        };
    }

    /**
     * @brief A fragment is a collection of elements that can be inserted into a parent without an enclosing tag.
     *
     * @tparam ChildrenT
     */
    template <typename... ParametersT>
    constexpr auto fragment(ParametersT&&... params)
    {
        return [generator = HtmlElement<Detail::fragment_>{}(std::forward<ParametersT>(params)...)](
                   auto& parentElement, Renderer const&) {
            return generator(parentElement, Renderer{.type = RendererType::Inplace});
        };
    }
}