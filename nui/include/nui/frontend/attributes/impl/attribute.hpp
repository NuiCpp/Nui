#pragma once

#include <nui/frontend/dom/element_fwd.hpp>
#include <nui/frontend/event_system/event_context.hpp>

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <variant>

namespace Nui
{
    class Attribute
    {
      public:
        struct RegularAttribute
        {
            std::function<void(Dom::ChildlessElement&)> setter;
            std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent;
            std::function<void(EventContext::EventIdType const&)> clearEvent;
        };
        struct StringDataAttribute
        {
            std::string data;
        };

        Attribute(
            std::function<void(Dom::ChildlessElement&)> setter,
            std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent = {},
            std::function<void(EventContext::EventIdType const&)> clearEvent = {})
            : attributeImpl_{RegularAttribute{
                  .setter = std::move(setter),
                  .createEvent = std::move(createEvent),
                  .clearEvent = std::move(clearEvent),
              }}
        {}
        Attribute(std::string data)
            : attributeImpl_{StringDataAttribute{
                  .data = std::move(data),
              }}
        {}
        Attribute(std::string_view data)
            : attributeImpl_{StringDataAttribute{
                  .data = std::string{data},
              }}
        {}

        Attribute(Attribute const&) = default;
        Attribute(Attribute&&) = default;
        Attribute& operator=(Attribute const&) = default;
        Attribute& operator=(Attribute&&) = default;

        void setOn(Dom::ChildlessElement& element) const;
        EventContext::EventIdType createEvent(std::weak_ptr<Dom::ChildlessElement>&& element) const;
        std::function<void(EventContext::EventIdType const&)> getEventClear() const;

        std::string const& stringData() const;

        bool isRegular() const;
        bool isStringData() const;

      private:
        std::variant<RegularAttribute, StringDataAttribute> attributeImpl_;
    };
}
