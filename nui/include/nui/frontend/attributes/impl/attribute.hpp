#pragma once

#include <nui/frontend/dom/element_fwd.hpp>
#include <nui/frontend/event_system/event_context.hpp>

#include <functional>
#include <memory>

namespace Nui
{
    class Attribute
    {
      public:
        Attribute()
            : setter_{}
            , createEvent_{}
        {}
        Attribute(
            std::function<void(Dom::ChildlessElement&)> setter,
            std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent = {},
            std::function<void(EventContext::EventIdType const&)> clearEvent = {})
            : setter_{std::move(setter)}
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

      private:
        std::function<void(Dom::ChildlessElement&)> setter_;
        std::function<EventContext::EventIdType(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent_;
        std::function<void(EventContext::EventIdType const&)> clearEvent_;
    };
}
