#pragma once

#include <nui/elements/html_element.hpp>

#include <string>
#include <vector>

namespace Nui
{
    struct SynthesizedAttribute
    {
        char const* key;
        std::string value;
    };

    class DomElement
    {
      public:
        template <typename T, typename... Attributes>
        DomElement(HtmlElement<T, Attributes...> const& element)
            : type_{T::name}
            , synthesized_attributes_{std::apply(
                  [](auto&&... attributes) {
                      return std::vector<SynthesizedAttribute>{SynthesizedAttribute{
                          std::decay_t<decltype(attributes)>::discrete_attributes::name, attributes.value()}...};
                  },
                  element.attributes_)}
        {}

        void render(DomElement const& child)
        {
            // TODO:
        }

      private:
        char const* type_;
        std::vector<SynthesizedAttribute> synthesized_attributes_;
    };
}