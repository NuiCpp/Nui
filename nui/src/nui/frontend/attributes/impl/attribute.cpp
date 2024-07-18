#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/dom/childless_element.hpp>

#include <iostream>

namespace Nui
{
    // #####################################################################################################################
    void Attribute::setOn(Dom::ChildlessElement& element) const
    {
        std::get<RegularAttribute>(attributeImpl_).setter(element);
    }
    //---------------------------------------------------------------------------------------------------------------------
    EventContext::EventIdType Attribute::createEvent(std::weak_ptr<Dom::ChildlessElement>&& element) const
    {
        if (createEvent_)
            return createEvent_(std::move(element));
        return EventContext::EventIdType{};
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::function<void(EventContext::EventIdType const&)> Attribute::getEventClear() const
    {
        return clearEvent_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string const& Attribute::stringData() const
    {
        return std::get<StringDataAttribute>(attributeImpl_).data;
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Attribute::isRegular() const
    {
        return std::holds_alternative<RegularAttribute>(attributeImpl_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Attribute::isStringData() const
    {
        return std::holds_alternative<StringDataAttribute>(attributeImpl_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    bool Attribute::defer() const
    {
        return defer_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Attribute::defer(bool doDefer) &
    {
        defer_ = doDefer;
    }
    // #####################################################################################################################
}