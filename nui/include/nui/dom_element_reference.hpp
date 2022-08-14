#pragma once

#include <nui/dom_element.hpp>

namespace Nui
{
    class DomElementReference
    {
      public:
        DomElementReference() = default;
        DomElementReference(const DomElementReference&) = default;
        DomElementReference(DomElementReference&&) = default;
        DomElementReference& operator=(const DomElementReference&) = default;
        DomElementReference& operator=(DomElementReference&&) = default;
        ~DomElementReference() = default;

        // TODO: Implement the following, but if the weak_ptr is invalid, just do nothing.
        // setAttribute(key, value)
        // addChild(DomElement)
        // clearChildren()
        // begin()
        // end()

      private:
        std::weak_ptr<DomElement> element_;
    };
}