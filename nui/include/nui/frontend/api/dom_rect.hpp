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

        static DomRect fromRect();
        static DomRect fromRect(RectObject);
    };
}