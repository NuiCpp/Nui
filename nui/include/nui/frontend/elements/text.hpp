#pragma once

#include <nui/frontend/attributes/impl/attribute_factory.hpp>
#include <nui/frontend/elements/impl/html_element_incl.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/frontend/event_system/event_context.hpp>

#include <string_view>

namespace Nui::Elements
{
    struct text : HtmlElement
    {
        text(text const&) = default;
        text(text&&) = default;
        text(std::string_view content)
            : HtmlElement{"", &TextElementBridge, Attribute{content, {}, {}}}
        {}
        text(Nui::Observed<std::string>& obs)
            : HtmlElement{
                  "",
                  &TextElementBridge,
                  Nui::Attributes::ElementMemberFactory<::Nui::Attributes::Detail::TextNodeAttributePolicy>{""} = obs,
              }
        {}
        template <typename... CombinatorParameters>
        text(Nui::ObservedValueCombinatorWithGenerator<CombinatorParameters...> combinator)
            : HtmlElement{
                  "",
                  &TextElementBridge,
                  Nui::Attributes::ElementMemberFactory<::Nui::Attributes::Detail::TextNodeAttributePolicy>{""} =
                      std::move(combinator),
              }
        {}
        text(std::shared_ptr<Nui::Observed<std::string>> obs)
            : HtmlElement{
                  "",
                  &TextElementBridge,
                  Nui::Attributes::ElementMemberFactory<::Nui::Attributes::Detail::TextNodeAttributePolicy>{""} =
                      std::move(obs),
              }
        {}
        text(std::weak_ptr<Nui::Observed<std::string>> const& obs)
            : text{obs.lock()}
        {}
    };
}