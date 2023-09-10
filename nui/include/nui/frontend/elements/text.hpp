#pragma once

#include <nui/frontend/elements/impl/html_element_incl.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/event_context.hpp>

#include <string_view>

namespace Nui::Elements
{
    struct text : HtmlElement
    {
        HTML_ELEMENT_CONSTEXPR text(text const&) = default;
        HTML_ELEMENT_CONSTEXPR text(text&&) = default;
        HTML_ELEMENT_CONSTEXPR text(std::string_view content)
            : HtmlElement{"", &TextElementBridge, Attribute{content}}
        {}
        HTML_ELEMENT_CONSTEXPR text(Nui::Observed<std::string>& obs)
            : HtmlElement{
                  "",
                  &TextElementBridge,
                  Attribute{
                      obs.value(),
                      [&obs](std::weak_ptr<Dom::ChildlessElement>&& element) {
                          const auto eventId = globalEventContext.registerEvent(Event{
                              [element, &obs](auto eventId) {
                                  if (auto shared = element.lock(); shared)
                                  {
                                      shared->setNodeValue(obs.value());
                                      return true;
                                  }
                                  obs.unattachEvent(eventId);
                                  return false;
                              },
                              [element]() {
                                  return !element.expired();
                              }});
                          obs.attachEvent(eventId);
                          return eventId;
                      },
                      [&obs](EventContext::EventIdType const& id) {
                          obs.unattachEvent(id);
                      },
                  },
              }
        {}
    };
}