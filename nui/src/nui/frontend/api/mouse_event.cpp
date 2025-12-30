#include <nui/frontend/api/mouse_event.hpp>

namespace Nui::WebApi
{
    MouseEvent::MouseEvent(Nui::val event)
        : UiEvent{std::move(event)}
    {}

    bool MouseEvent::altKey() const
    {
        return val_["altKey"].as<bool>();
    }
    int MouseEvent::button() const
    {
        return val_["button"].as<int>();
    }
    int MouseEvent::buttons() const
    {
        return val_["buttons"].as<int>();
    }
    double MouseEvent::clientX() const
    {
        return val_["clientX"].as<double>();
    }
    double MouseEvent::clientY() const
    {
        return val_["clientY"].as<double>();
    }
    double MouseEvent::x() const
    {
        return val_["x"].as<double>();
    }
    double MouseEvent::y() const
    {
        return val_["y"].as<double>();
    }
    double MouseEvent::movementX() const
    {
        return val_["movementX"].as<double>();
    }
    double MouseEvent::movementY() const
    {
        return val_["movementY"].as<double>();
    }
    double MouseEvent::offsetX() const
    {
        return val_["offsetX"].as<double>();
    }
    double MouseEvent::offsetY() const
    {
        return val_["offsetY"].as<double>();
    }
    double MouseEvent::pageX() const
    {
        return val_["pageX"].as<double>();
    }
    double MouseEvent::pageY() const
    {
        return val_["pageY"].as<double>();
    }
    double MouseEvent::screenX() const
    {
        return val_["screenX"].as<double>();
    }
    double MouseEvent::screenY() const
    {
        return val_["screenY"].as<double>();
    }
    bool MouseEvent::ctrlKey() const
    {
        return val_["ctrlKey"].as<bool>();
    }
    bool MouseEvent::shiftKey() const
    {
        return val_["shiftKey"].as<bool>();
    }
    bool MouseEvent::metaKey() const
    {
        return val_["metaKey"].as<bool>();
    }
    std::optional<Nui::val> MouseEvent::relatedTarget() const
    {
        Nui::val rt = val_["relatedTarget"];
        if (rt.isUndefined() || rt.isNull())
            return std::nullopt;
        return rt;
    }
}