#pragma once

#include <nui/frontend/dom/element_fwd.hpp>

#include <functional>

namespace Nui
{
    class Attribute
    {
      public:
        Attribute()
            : setter_{}
            , createEvent_{}
        {}
        Attribute(
            std::function<void(Dom::ChildlessElement&)> setter,
            std::function<void(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent = {})
            : setter_{std::move(setter)}
            , createEvent_{std::move(createEvent)}
        {}

        Attribute(Attribute const&) = default;
        Attribute(Attribute&&) = default;
        Attribute& operator=(Attribute const&) = default;
        Attribute& operator=(Attribute&&) = default;

        void setOn(Dom::ChildlessElement& element) const;
        void createEvent(std::weak_ptr<Dom::ChildlessElement>&& element) const;

      private:
        std::function<void(Dom::ChildlessElement&)> setter_;
        std::function<void(std::weak_ptr<Dom::ChildlessElement>&& element)> createEvent_;
    };
}
