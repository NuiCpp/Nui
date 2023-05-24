#pragma once

#include <emscripten/val.h>

#include <memory>

namespace Nui::Dom
{
    class BasicElement : public std::enable_shared_from_this<BasicElement>
    {
      public:
        BasicElement(emscripten::val val)
            : element_{std::move(val)}
        {}

        emscripten::val const& val() const
        {
            return element_;
        }
        emscripten::val& val()
        {
            return element_;
        }
        operator emscripten::val const&() const
        {
            return element_;
        }
        operator emscripten::val&()
        {
            return element_;
        }
        operator emscripten::val&&() &&
        {
            return std::move(element_);
        }

        template <class Derived>
        std::shared_ptr<Derived> shared_from_base()
        {
            return std::static_pointer_cast<Derived>(shared_from_this());
        }
        template <class Derived>
        std::weak_ptr<Derived> weak_from_base()
        {
            return std::weak_ptr<Derived>(shared_from_base<Derived>());
        }

      protected:
        emscripten::val element_;
    };
}