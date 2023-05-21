#pragma once

#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/utility/tuple_for_each.hpp>

#include <emscripten/val.h>

#include <concepts>
#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace Nui::Dom
{
    class Element : public ChildlessElement
    {
      public:
        using collection_type = std::vector<std::shared_ptr<Element>>;
        using iterator = collection_type::iterator;
        using const_iterator = collection_type::const_iterator;

        Element(HtmlElement const& elem)
            : ChildlessElement{elem}
            , children_{}
        {}
        virtual ~Element()
        {
            element_.call<void>("remove");
        }

        template <typename... Attributes>
        static std::shared_ptr<Element> makeElement(HtmlElement const& element)
        {
            auto elem = std::make_shared<Element>(element);
            elem->setup(element);
            return elem;
        }

        Element(emscripten::val val)
            : ChildlessElement{std::move(val)}
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

        void appendElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Append});
        }
        auto appendElement(HtmlElement const& element)
        {
            auto elem = makeElement(element);
            element_.call<emscripten::val>("appendChild", elem->element_);
            return children_.emplace_back(std::move(elem));
        }
        void replaceElement(std::invocable<Element&, Renderer const&> auto&& fn)
        {
            fn(*this, Renderer{.type = RendererType::Replace});
        }
        auto replaceElement(HtmlElement const& element)
        {
            ChildlessElement::replaceElement(element);
            return shared_from_base<Element>();
        }

        void setTextContent(std::string const& text)
        {
            element_.set("textContent", text);
        }
        void setTextContent(char const* text)
        {
            element_.set("textContent", text);
        }

        void
        appendElements(std::vector<std::function<std::shared_ptr<Element>(Element&, Renderer const&)>> const& elements)
        {
            for (auto const& element : elements)
                appendElement(element);
        }

        auto insert(iterator where, HtmlElement const& element)
        {
            if (where == end())
                return appendElement(element);
            auto elem = makeElement(element);
            element_.call<emscripten::val>("insertBefore", elem->element_, (*where)->element_);
            return *children_.insert(where, std::move(elem));
        }

        auto insert(std::size_t where, HtmlElement const& element)
        {
            if (where >= children_.size())
                return appendElement(element);
            else
                return insert(begin() + static_cast<decltype(children_)::difference_type>(where), element);
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