#pragma once

#include <nui/frontend/dom/basic_element.hpp>
#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/utility/functions.hpp>

namespace Nui::Dom
{
    /**
     * @brief The basic element cannot have children and does not hold explicit ownership of them.
     * To represent an actual HtmlElement use the Element class.
     */
    class ChildlessElement : public BasicElement
    {
      public:
        template <typename... Attributes>
        ChildlessElement(HtmlElement<Attributes...> const& elem)
            : BasicElement{ChildlessElement::createElement(elem).val()}
        {}
        ChildlessElement(emscripten::val val)
            : BasicElement{std::move(val)}
        {}

        template <typename... Attributes>
        static ChildlessElement createElement(HtmlElement<Attributes...> const& element)
        {
            return {emscripten::val::global("document")
                        .call<emscripten::val>("createElement", emscripten::val{element.name()})};
        }

        /**
         * @brief Relies on weak_from_this and cannot be used from the constructor
         */
        template <typename... Attributes>
        void setup(HtmlElement<Attributes...> const& element)
        {
#pragma clang diagnostic push
// 'this' may be unused when the tuple is empty? anyway this warning cannot be fixed.
#pragma clang diagnostic ignored "-Wunused-lambda-capture"
            tupleForEach(element.attributes(), [self = this](auto const& attribute) {
                attribute.setOn(*self);
                attribute.createEvent(self->weak_from_base<ChildlessElement>());
            });
#pragma clang diagnostic pop
        }

        // TODO: more overloads?
        void setAttribute(std::string_view key, std::string const& value)
        {
            // FIXME: performance, keys are turned to std::string
            if (value.empty())
                element_.call<emscripten::val>("removeAttribute", emscripten::val{std::string{key}});
            else
                element_.call<emscripten::val>(
                    "setAttribute", emscripten::val{std::string{key}}, emscripten::val{value});
        }
        void setAttribute(std::string_view key, std::invocable<emscripten::val> auto&& value)
        {
            element_.set(emscripten::val{std::string{key}}, Nui::bind(value, std::placeholders::_1));
        }
        void setAttribute(std::string_view key, char const* value)
        {
            if (value[0] == '\0')
                element_.call<emscripten::val>("removeAttribute", emscripten::val{std::string{key}});
            else
                element_.call<emscripten::val>(
                    "setAttribute", emscripten::val{std::string{key}}, emscripten::val{std::string{value}});
        }
        void setAttribute(std::string_view key, bool value)
        {
            if (value)
                element_.call<emscripten::val>("setAttribute", emscripten::val{std::string{key}}, emscripten::val{""});
        }
        void setAttribute(std::string_view key, int value)
        {
            element_.call<emscripten::val>("setAttribute", emscripten::val{std::string{key}}, emscripten::val{value});
        }
        void setAttribute(std::string_view key, double value)
        {
            element_.call<emscripten::val>("setAttribute", emscripten::val{std::string{key}}, emscripten::val{value});
        }
        void setAttribute(std::string_view key, emscripten::val value)
        {
            element_.call<emscripten::val>("setAttribute", emscripten::val{std::string{key}}, value);
        }
        template <typename T>
        void setAttribute(std::string_view key, std::optional<T> const& value)
        {
            if (value)
                setAttribute(key, *value);
        }

      protected:
        template <typename... Attributes>
        void replaceElement(HtmlElement<Attributes...> const& element)
        {
            auto replacement = ChildlessElement::createElement(element);
            replacement.setup(element);
            element_.call<emscripten::val>("replaceWith", replacement.val());
            element_ = std::move(replacement).val();
        }
    };
};
