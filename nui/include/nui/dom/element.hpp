#pragma once

#include <nui/elements/html_element.hpp>
#include <nui/utility/inferance_helper.hpp>

#include <emscripten/val.h>

#include <string>
#include <vector>
#include <memory>

namespace Nui::Dom
{
    class Element
    {
      public:
        using iterator = std::vector<std::shared_ptr<Element>>::iterator;
        using const_iterator = std::vector<std::shared_ptr<Element>>::const_iterator;

        template <typename T, typename... Attributes>
        Element(HtmlElement<T, Attributes...> const& element)
            : type_{T::name}
            , element_{emscripten::val::global("document").call<emscripten::val>("createElement", type_)}
            , children_{}
        {
            for (auto const& attribute : element.attributes)
            {
                element_.set(attribute.name, attribute.value);
            }
        }

        template <typename T>
        Element(Utility::InferanceHelper<T>, emscripten::val val)
            : type_{T::name}
            , element_{std::move(val)}
            , children_{}
        {}

        // TODO: more overloads?
        void setAttribute(std::string_view key, std::string_view value)
        {
            element_.call<emscripten::val>("setAttribute", key, value);
        }

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

        template <typename U, typename... Attributes>
        void appendElement(HtmlElement<U, Attributes...> const& element)
        {
            children_.emplace_back(std::make_shared<Element>(element));
            element_.call<emscripten::val>("appendChild", element);
        }

        template <typename U, typename... Attributes>
        void insert(iterator where, HtmlElement<U, Attributes...> const& element)
        {
            element_.call<emscripten::val>("insertBefore", element, where->element_);
            children_.insert(where, Element{element});
        }

      private:
        char const* type_;
        emscripten::val element_;
        std::vector<std::shared_ptr<Element>> children_;
    };
}