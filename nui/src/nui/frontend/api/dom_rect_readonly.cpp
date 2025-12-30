#include <nui/frontend/api/dom_rect_readonly.hpp>

namespace Nui::WebApi
{
    DomRectReadOnly::DomRectReadOnly(Nui::val val)
        : ValWrapper{std::move(val)}
    {}

    double DomRectReadOnly::x() const
    {
        return val_["x"].as<double>();
    }
    double DomRectReadOnly::y() const
    {
        return val_["y"].as<double>();
    }
    double DomRectReadOnly::width() const
    {
        return val_["width"].as<double>();
    }
    double DomRectReadOnly::height() const
    {
        return val_["height"].as<double>();
    }
    double DomRectReadOnly::top() const
    {
        return val_["top"].as<double>();
    }
    double DomRectReadOnly::right() const
    {
        return val_["right"].as<double>();
    }
    double DomRectReadOnly::bottom() const
    {
        return val_["bottom"].as<double>();
    }
    double DomRectReadOnly::left() const
    {
        return val_["left"].as<double>();
    }

    DomRectReadOnly DomRectReadOnly::fromRect()
    {
        return DomRectReadOnly{Nui::val::global("DOMRectReadOnly").call<Nui::val>("fromRect")};
    }
    DomRectReadOnly DomRectReadOnly::fromRect(RectObject rect)
    {
        Nui::val object = Nui::val::object();
        object.set("x", rect.x);
        object.set("y", rect.y);
        object.set("width", rect.width);
        object.set("height", rect.height);
        return DomRectReadOnly{Nui::val::global("DOMRectReadOnly").call<Nui::val>("fromRect", object)};
    }
}