#pragma once

#include <nui/frontend/val.hpp>

#include <algorithm>
#include <cctype>
#include <memory>

namespace Nui::Dom
{
    class BasicElement : public std::enable_shared_from_this<BasicElement>
    {
      public:
        BasicElement(Nui::val val)
            : element_{std::move(val)}
        {}
        virtual ~BasicElement() = default;

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
        std::string tagName() const
        {
            auto tag = element_["tagName"].as<std::string>();
            std::transform(tag.begin(), tag.end(), tag.begin(), [](unsigned char c) {
                return std::tolower(c);
            });
            return tag;
        }

      protected:
        Nui::val element_;
    };
}