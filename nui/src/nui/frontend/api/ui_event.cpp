#include <nui/frontend/api/ui_event.hpp>

namespace Nui::WebApi
{
    UiEvent::UiEvent(Nui::val event)
        : Event{std::move(event)}
    {}
    long UiEvent::detail() const
    {
        return val_["detail"].as<long>();
    }
    Nui::val UiEvent::sourceCapabilities() const
    {
        return val_["sourceCapabilities"];
    }
    Nui::val UiEvent::view() const
    {
        return val_["view"];
    }
}