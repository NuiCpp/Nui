#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/frontend/api/console.hpp>

#include <map>
#include <optional>
#include <string>
#include <functional>

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
        return EventContext::invalidEventId;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::function<void(EventContext::EventIdType const&)> Attribute::getEventClear() const
    {
        return clearEvent_;
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::string Attribute::stringData() const
    {
        return std::visit(
            [](auto&& arg) -> std::string {
                using T = std::decay_t<decltype(arg)>;
                if constexpr (std::is_same_v<T, StringDataAttribute>)
                    return arg.data;
                else if constexpr (std::is_same_v<T, RegularAttribute>)
                {
                    auto textVal = Nui::val::object();
                    Dom::ChildlessElement elem{textVal};
                    arg.setter(elem);
                    return textVal["nodeValue"].as<std::string>();
                }
                else
                {
                    return {};
                }
            },
            attributeImpl_);
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