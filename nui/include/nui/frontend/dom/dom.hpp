#pragma once

#include <nui/core.hpp>
#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/dom/element.hpp>

#include <memory>

namespace Nui::Dom
{
    class Dom
    {
      public:
        Dom();
        Dom(const Dom&) = default;
        Dom(Dom&&) = default;
        Dom& operator=(const Dom&) = default;
        Dom& operator=(Dom&&) = default;
        ~Dom() = default;

        Element& root();
        template <typename T>
        void setBody(T&& body)
        {
            root().replaceElement(std::forward<T>(body));
        }

      private:
        std::shared_ptr<Element> root_;
    };
}
