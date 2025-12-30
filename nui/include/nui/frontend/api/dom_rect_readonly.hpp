#pragma once

#include <nui/frontend/val_wrapper.hpp>

namespace Nui::WebApi
{
    class DomRectReadOnly : public ValWrapper
    {
      public:
        explicit DomRectReadOnly(Nui::val val);

        double x() const;
        double y() const;
        double width() const;
        double height() const;
        double top() const;
        double right() const;
        double bottom() const;
        double left() const;

        static DomRectReadOnly fromRect();
        struct RectObject
        {
            double x = 0.;
            double y = 0.;
            double width = 0.;
            double height = 0.;
        };
        static DomRectReadOnly fromRect(RectObject);
    };
}