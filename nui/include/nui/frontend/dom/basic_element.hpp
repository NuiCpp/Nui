#pragma once

#include <nui/frontend/val.hpp>

#include <memory>

namespace Nui::Dom
{
    class BasicElement : public std::enable_shared_from_this<BasicElement>
    {
      public:
        BasicElement(Nui::val val)
            : element_{std::move(val)}
        {}

        Nui::val const& val() const
        {
            return element_;
        }
        Nui::val& val()
        {
            return element_;
        }
        operator Nui::val const&() const
        {
            return element_;
        }
        operator Nui::val&()
        {
            return element_;
        }
        operator Nui::val&&() &&
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
        Nui::val element_;
    };
}