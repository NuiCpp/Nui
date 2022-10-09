#pragma once

#include <nui/elements/impl/html_element.hpp>
#include <nui/core.hpp>
#include <nui/dom/element.hpp>

#include <optional>

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
