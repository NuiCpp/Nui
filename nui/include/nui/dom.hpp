#pragma once

#include <nui/elements/html_element.hpp>
#include <nui/core.hpp>
#include <nui/dom_element.hpp>

#include <stlab/forest.hpp>

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

namespace Nui
{
    class Dom
    {
      public:
        Dom() = default;
        Dom(const Dom&) = default;
        Dom(Dom&&) = default;
        Dom& operator=(const Dom&) = default;
        Dom& operator=(Dom&&) = default;
        ~Dom() = default;

        template <typename... Elements>
        void root(Elements&&... elements)
        {
            elements_.clear();
            (..., insert(elements_.end(), elements));
        }

      private:
        template <typename T, typename... Attributes>
        void insert(auto const& iterator, HtmlElement<T, Attributes...> const& element)
        {
            iterator->render(*elements_.insert(iterator, DomElement{element}));
        }
        template <typename T, typename... Attributes>
        void insert(auto const& iterator, HtmlElement<T, Attributes...>&& element)
        {
            iterator->render(*elements_.insert(iterator, DomElement{std::move(element)}));
        }
        template <typename... Elements>
        void insert(auto const& iterator, std::tuple<Elements...> const& elements)
        {
            std::apply(
                [this, &iterator](auto const&... elements) {
                    (..., insert(iterator, elements));
                },
                elements);
        }
        template <typename... Elements>
        void insert(auto const& iterator, std::tuple<Elements...>&& elements)
        {
            std::apply(
                [this, &iterator](auto&&... elements) {
                    (..., insert(iterator, elements));
                },
                std::move(elements));
        }

      private:
        stlab::forest<DomElement> elements_;
    };
}
