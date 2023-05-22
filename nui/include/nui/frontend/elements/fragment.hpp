#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>

#include <tuple>

namespace Nui::Elements
{
    /**
     * @brief A fragment is a collection of elements that can be inserted into a parent without an enclosing tag.
     *
     * @tparam ChildrenT
     */
    template <typename... ParametersT>
    constexpr auto fragment(ParametersT&&... params)
    {
        return [generator = HtmlElement{"fragmenterror"}(std::forward<ParametersT>(params)...)](
                   auto& parentElement, Renderer const&) {
            return generator(parentElement, Renderer{.type = RendererType::Inplace});
        };
    }
}