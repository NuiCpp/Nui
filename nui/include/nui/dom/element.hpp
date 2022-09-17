#pragma once

#include <nui/elements/html_element.hpp>
#include <nui/utility/functions.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/utility/tuple_for_each.hpp>
#include <nui/dom/basic_element.hpp>

#include <emscripten/val.h>

#include <concepts>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Nui::Dom
{
    class Element : public BasicElement
    {
      public:
        using collection_type = std::vector<std::shared_ptr<Element>>;
        using iterator = collection_type::iterator;
        using const_iterator = collection_type::const_iterator;

        template <typename T, typename... Attributes>
        Element(HtmlElement<T, Attributes...> const& elem)
            : BasicElement{elem}
            , children_{}
        {}
        virtual ~Element()
        {
            element_.call<void>("remove");
        }

        template <typename U, typename... Attributes>
        static std::shared_ptr<Element> makeElement(HtmlElement<U, Attributes...> const& element)
        {
            auto elem = std::make_shared<Element>(element);
            elem->setup(element);
            return elem;
        }

        Element(emscripten::val val)
            : BasicElement{std::move(val)}
            , children_{}
        {}

        iterator begin()
        {
            return std::begin(children_);
        }
        iterator end()
        {
            return std::end(children_);
        }
        const_iterator begin() const
        {
            return std::begin(children_);
        }
        const_iterator end() const
        {
            return std::end(children_);
        }

        void appendElement(std::invocable<Element&, GeneratorOptions const&> auto&& fn)
        {
            fn(*this, {});
        }

        template <typename U, typename... Attributes>
        auto appendElement(HtmlElement<U, Attributes...> const& element)
        {
            auto elem = makeElement(element);
            element_.call<emscripten::val>("appendChild", elem->element_);
            return children_.emplace_back(std::move(elem));
        }

        template <typename U, typename... Attributes>
        auto replaceElement(HtmlElement<U, Attributes...> const& element)
        {
            BasicElement::replaceElement(element);
            return shared_from_base<Element>();
        }

        template <typename... Elements>
        void appendElements(std::tuple<Elements...> const& elements)
        {
            std::apply(
                [this](auto const&... element) {
                    (appendElement(element), ...);
                },
                elements);
        }

        template <typename U, typename... Attributes>
        void insert(iterator where, HtmlElement<U, Attributes...> const& element)
        {
            auto elem = makeElement(element);
            element_.call<emscripten::val>("insertBefore", elem->element_, (*where)->element_);
            children_.insert(where, std::move(elem));
        }

        auto& operator[](std::size_t index)
        {
            return children_[index];
        }
        auto const& operator[](std::size_t index) const
        {
            return children_[index];
        }

        auto erase(iterator where)
        {
            return children_.erase(where);
        }

        void clearChildren()
        {
            children_.clear();
        }

      private:
        collection_type children_;
    };
}