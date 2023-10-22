#pragma once

#include <nui/frontend/dom/basic_element.hpp>
#include <nui/frontend/elements/impl/html_element.hpp>
#include <nui/frontend/utility/functions.hpp>

#include <optional>
#include <string>
#include <string_view>
#include <variant>

namespace Nui::Dom
{
    /**
     * @brief The basic element cannot have children and does not hold explicit ownership of them.
     * To represent an actual HtmlElement use the Element class.
     */
    class ChildlessElement : public BasicElement
    {
      public:
        explicit ChildlessElement(HtmlElement const& elem)
            : BasicElement{ChildlessElement::createElement(elem)}
        {}
        explicit ChildlessElement(Nui::val val)
            : BasicElement{std::move(val)}
        {}

        void setProperty(std::string_view key, std::string const& value)
        {
            element_.set(Nui::val{std::string{key}}, Nui::val{value});
        }
        void setProperty(std::string_view key, std::string_view value)
        {
            setProperty(key, std::string{value});
        }
        void setProperty(std::string_view key, char const* value)
        {
            if (value[0] == '\0')
                element_.delete_(std::string{key});
            else
                setProperty(key, std::string{value});
        }
        void setProperty(std::string_view key, std::invocable<Nui::val> auto&& value)
        {
            element_.set(Nui::val{std::string{key}}, Nui::bind(value, std::placeholders::_1));
        }
        template <typename T>
        void setProperty(std::string_view key, std::optional<T> const& value)
        {
            if (value)
                setProperty(key, *value);
            else
                element_.delete_(std::string{key});
        }
        void setProperty(std::string_view key, bool value)
        {
            element_.set(Nui::val{std::string{key}}, Nui::val{value});
        }
        template <typename... List>
        void setProperty(std::string_view key, std::variant<List...> const& variant)
        {
            std::visit(
                [this, &key](auto const& value) {
                    this->setProperty(key, value);
                },
                variant);
        }
        template <typename T>
        requires std::integral<T>
        void setProperty(std::string_view key, T value)
        {
            element_.set(Nui::val{std::string{key}}, Nui::val{static_cast<int>(value)});
        }
        template <typename T>
        requires std::floating_point<T>
        void setProperty(std::string_view key, T value)
        {
            element_.set(Nui::val{std::string{key}}, Nui::val{static_cast<double>(value)});
        }

        void addEventListener(std::string_view event, std::invocable<Nui::val> auto&& callback)
        {
            element_.call<void>(
                "addEventListener", Nui::val{std::string{event}}, Nui::bind(callback, std::placeholders::_1));
        }

        // TODO: more overloads?
        void setAttribute(std::string_view key, std::string const& value)
        {
            // FIXME: performance, keys are turned to std::string
            if (value.empty())
                element_.call<Nui::val>("removeAttribute", Nui::val{std::string{key}});
            else
                element_.call<Nui::val>("setAttribute", Nui::val{std::string{key}}, Nui::val{value});
        }
        void setAttribute(std::string_view key, std::string_view value)
        {
            setAttribute(key, std::string{value});
        }
        void setAttribute(std::string_view key, std::invocable<Nui::val> auto&& value)
        {
            element_.set(Nui::val{std::string{key}}, Nui::bind(value, std::placeholders::_1));
        }
        void setAttribute(std::string_view key, char const* value)
        {
            if (value[0] == '\0')
                element_.call<Nui::val>("removeAttribute", Nui::val{std::string{key}});
            else
                element_.call<Nui::val>("setAttribute", Nui::val{std::string{key}}, Nui::val{std::string{value}});
        }
        void setAttribute(std::string_view key, bool value)
        {
            if (value)
                element_.call<Nui::val>("setAttribute", Nui::val{std::string{key}}, Nui::val{std::string{key}});
            else
                element_.call<Nui::val>("removeAttribute", Nui::val{std::string{key}});
        }
        template <typename T>
        requires std::integral<T>
        void setAttribute(std::string_view key, T value)
        {
            element_.call<Nui::val>("setAttribute", Nui::val{std::string{key}}, Nui::val{static_cast<int>(value)});
        }
        template <typename T>
        requires std::floating_point<T>
        void setAttribute(std::string_view key, T value)
        {
            element_.call<Nui::val>("setAttribute", Nui::val{std::string{key}}, Nui::val{static_cast<double>(value)});
        }
        void setAttribute(std::string_view key, Nui::val value)
        {
            element_.call<Nui::val>("setAttribute", Nui::val{std::string{key}}, value);
        }
        template <typename T>
        void setAttribute(std::string_view key, std::optional<T> const& value)
        {
            if (value)
                setAttribute(key, *value);
            else
                element_.call<Nui::val>("removeAttribute", Nui::val{std::string{key}});
        }
        template <typename... List>
        void setAttribute(std::string_view key, std::variant<List...> const& variant)
        {
            std::visit(
                [this, &key](auto const& value) {
                    this->setAttribute(key, value);
                },
                variant);
        }

        void setNodeValue(std::string_view value)
        {
            element_.set("nodeValue", Nui::val{std::string{value}});
        }
        void setNodeValue(std::string const& value)
        {
            element_.set("nodeValue", Nui::val{value});
        }

      protected:
        static Nui::val createElement(HtmlElement const& element)
        {
            return element.bridge()->createElement(element);
        }
    };
};
