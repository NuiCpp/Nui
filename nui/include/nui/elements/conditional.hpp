#pragma once

#include <nui/elements/html_element.hpp>

namespace Nui
{
    /**
     * @brief A conditional is for generating either A or B depending on a condition.
     *
     * @tparam ChildrenT
     */
    template <typename... ChildrenT>
    class conditional
    {
      public:
        template <typename... ElementT>
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return [children = std::make_tuple(std::forward<ElementT>(elements)...)]<typename T>(
                       auto& parentElement, RendererOptions<T> const& options) {
                parentElement.appendElements(children);
                return parentElement.shared_from_this();
            };
        }
    };
}