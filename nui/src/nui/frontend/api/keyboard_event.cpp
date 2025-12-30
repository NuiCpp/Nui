#include <nui/frontend/api/keyboard_event.hpp>

namespace Nui::WebApi
{
    KeyboardEvent::KeyboardEvent(Nui::val event)
        : UiEvent{std::move(event)}
    {}
    bool KeyboardEvent::altKey() const
    {
        return val_["altKey"].as<bool>();
    }

    bool KeyboardEvent::ctrlKey() const
    {
        return val_["ctrlKey"].as<bool>();
    }

    bool KeyboardEvent::shiftKey() const
    {
        return val_["shiftKey"].as<bool>();
    }

    std::string KeyboardEvent::code() const
    {
        return val_["code"].as<std::string>();
    }

    bool KeyboardEvent::isComposing() const
    {
        return val_["isComposing"].as<bool>();
    }

    std::string KeyboardEvent::key() const
    {
        return val_["key"].as<std::string>();
    }

    int KeyboardEvent::location() const
    {
        return val_["location"].as<int>();
    }

    bool KeyboardEvent::metaKey() const
    {
        return val_["metaKey"].as<bool>();
    }

    bool KeyboardEvent::repeat() const
    {
        return val_["repeat"].as<bool>();
    }
}