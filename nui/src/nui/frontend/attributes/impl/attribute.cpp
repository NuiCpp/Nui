#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/dom/childless_element.hpp>

#include <iostream>

namespace Nui
{
    void Attribute::setOn(Dom::ChildlessElement& element) const
    {
        if (setter_)
            setter_(element);
    }

    EventContext::EventIdType Attribute::createEvent(std::weak_ptr<Dom::ChildlessElement>&& element) const
    {
        if (createEvent_)
            return createEvent_(std::move(element));
        return EventContext::EventIdType{};
    }

    std::function<void(EventContext::EventIdType const&)> Attribute::getEventClear() const
    {
        return clearEvent_;
    }
}