#pragma once

#include <nui/elements/html_element.hpp>
#include <nui/core.hpp>
#include <nui/dom/element.hpp>

#include <optional>

/*
void render()
{
    return <div>
        <button></button>
    </div>;

    // auto x1 = p.addElement(div)
    // x1.addElement(button)
}
*/

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

      private:
        Element root_;
    };
}
