#pragma once

#include <nui/frontend/elements/impl/html_element_incl.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
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
                                  obs.detachEvent(eventId);
                                  return false;
                              },
                              [element]() {
                                  return !element.expired();
                              }});
                          obs.attachEvent(eventId);
                          return eventId;
                      },
                      [&obs](EventContext::EventIdType const& id) {
                          obs.detachEvent(id);
                      },
                  },
              }
        {}
    };
}