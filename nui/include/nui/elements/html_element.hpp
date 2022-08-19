#pragma once

#include <tuple>
#include <utility>

namespace Nui
{
    template <typename Derived, typename... Attributes>
    class HtmlElement
    {
      public:
        friend class DomElement;

        constexpr HtmlElement(Attributes&&... attributes)
            : attributes_{std::move(attributes)...}
        {}

        template <typename... ElementT>
        constexpr auto operator()(ElementT&&... elements) &&
        {
            return [this, children = std::make_tuple(std::forward<ElementT>(elements)...)](auto& materializedElement) {
                materializedElement.appendElement(*this)->appendElements(children);
            };
        }

        std::tuple<Attributes...> const& attributes() const
        {
            return attributes_;
        }

      private:
        std::tuple<Attributes...> attributes_;
    };
}