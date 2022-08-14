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
        using collection_type = std::vector<std::shared_ptr<Element>>;
        using iterator = collection_type::iterator;
        using const_iterator = collection_type::const_iterator;

        template <typename T, typename... Attributes>
        Element(HtmlElement<T, Attributes...> const& element)
            : type_{T::name}
            , element_{emscripten::val::global("document")
                           .call<emscripten::val>("createElement", emscripten::val{type_})}
            , children_{}
        {
            std::apply(
                [this](auto const&... attribute) {
                    (..., element_.set(attribute.name(), attribute.value()));
                },
                element.attributes());
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
            auto elem = std::make_shared<Element>(element);
            element_.call<emscripten::val>("appendChild", elem->element_);
            children_.emplace_back(std::move(elem));
        }

        template <typename... Elements>
        void appendElement(std::tuple<Elements...> const& elements)
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
            auto elem = std::make_shared<Element>(element);
            element_.call<emscripten::val>("insertBefore", elem->element_, (*where)->element_);
            children_.insert(where, std::move(elem));
        }

        emscripten::val& val()
        {
            return element_;
        }

      private:
        char const* type_;
        emscripten::val element_;
        collection_type children_;
    };
}