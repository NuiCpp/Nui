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
        };
        struct StringDataAttribute
        {
            std::string data;
        };

        Attribute() = default;
        Attribute(
            std::function<void(Dom::ChildlessElement&)> setter,
            std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent = {},
            std::function<void(EventContext::EventIdType const&)> clearEvent = {})
            : attributeImpl_{RegularAttribute{.setter = std::move(setter)}}
            , createEvent_{std::move(createEvent)}
            , clearEvent_{std::move(clearEvent)}
        {}
        Attribute(
            std::string data,
            std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent,
            std::function<void(EventContext::EventIdType const&)> clearEvent)
            : attributeImpl_{StringDataAttribute{
                  .data = std::move(data),
              }}
            , createEvent_{std::move(createEvent)}
            , clearEvent_{std::move(clearEvent)}
        {}
        Attribute(
            std::string_view data,
            std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent,
            std::function<void(EventContext::EventIdType const&)> clearEvent)
            : attributeImpl_{StringDataAttribute{
                  .data = std::string{data},
              }}
            , createEvent_{std::move(createEvent)}
            , clearEvent_{std::move(clearEvent)}
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
        bool defer() const;
        void defer(bool doDefer) &;
        Attribute&& defer(bool doDefer) &&
        {
            defer(doDefer);
            return std::move(*this);
        }

      private:
        std::variant<std::monostate, RegularAttribute, StringDataAttribute> attributeImpl_{};
        std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent_{};
        std::function<void(EventContext::EventIdType const&)> clearEvent_{};
        bool defer_{false};
    };

    inline Attribute&& operator!(Attribute&& attribute)
    {
        attribute.defer(!attribute.defer());
        return std::move(attribute);
    }
}
