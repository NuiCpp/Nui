#pragma once

#include <nui/frontend/api/dom_rect_readonly.hpp>

#include <chrono>
#include <string>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
     */
    class DomRect : public DomRectReadOnly
    {
      public:
        explicit DomRect(Nui::val rect);

        void x(double newX);
        void y(double newY);
        void width(double newWidth);
        void height(double newHeight);

        double x() const
        {
            return DomRectReadOnly::x();
        }
        double y() const
        {
            return DomRectReadOnly::y();
        }
        double width() const
        {
            return DomRectReadOnly::width();
        }
        double height() const
        {
            return DomRectReadOnly::height();
        }
        double top() const
        {
            return DomRectReadOnly::top();
        }
        double right() const
        {
            return DomRectReadOnly::right();
        }
        double bottom() const
        {
            return DomRectReadOnly::bottom();
        }
        double left() const
        {
            return DomRectReadOnly::left();
        }

        static DomRect fromRect();
        static DomRect fromRect(RectObject);
    };
}