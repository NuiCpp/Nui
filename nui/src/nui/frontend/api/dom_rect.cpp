#include <nui/frontend/api/dom_rect.hpp>

namespace Nui::WebApi
{
    DomRect::DomRect(Nui::val rect)
        : DomRectReadOnly{std::move(rect)}
    {}

    void DomRect::x(double newX)
    {
        val_.set("x", newX);
    }
    void DomRect::y(double newY)
    {
        val_.set("y", newY);
    }
    void DomRect::width(double newWidth)
    {
        val_.set("width", newWidth);
    }
    void DomRect::height(double newHeight)
    {
        val_.set("height", newHeight);
    }
    DomRect DomRect::fromRect()
    {
        return DomRect{Nui::val::global("DOMRect").call<Nui::val>("fromRect")};
    }
    DomRect DomRect::fromRect(RectObject rect)
    {
        Nui::val object = Nui::val::object();
        object.set("x", rect.x);
        object.set("y", rect.y);
        object.set("width", rect.width);
        object.set("height", rect.height);
        return DomRect{Nui::val::global("DOMRect").call<Nui::val>("fromRect", object)};
    }
}