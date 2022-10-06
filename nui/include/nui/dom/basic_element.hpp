#pragma once

#include <nui/elements/html_element.hpp>

#include <emscripten/val.h>

namespace Nui
{
    /**
     * @brief The basic element cannot have children and does not hold explicit ownership of them.
     * To represent an actual HtmlElement use the Element class.
     */
    class BasicElement : public std::enable_shared_from_this<BasicElement>
    {
      public:
        template <typename T, typename... Attributes>
        BasicElement(HtmlElement<T, Attributes...> const& elem)
            : element_{BasicElement::createElement(elem).val()}
        {}
        BasicElement(emscripten::val val)
            : element_{std::move(val)}
        {}
        template <class Derived>
        std::shared_ptr<Derived> shared_from_base()
        {
            return std::static_pointer_cast<Derived>(shared_from_this());
        }
        template <class Derived>
        std::weak_ptr<Derived> weak_from_base()
        {
            return std::weak_ptr<Derived>(shared_from_base<Derived>());
        }

        template <typename T, typename... Attributes>
        static BasicElement createElement(HtmlElement<T, Attributes...> const&)
        {
            return {
                emscripten::val::global("document").call<emscripten::val>("createElement", emscripten::val{T::name})};
        }

        /**
         * @brief Relies on weak_from_this and cannot be used from the constructor
         */
        template <typename T, typename... Attributes>
        void setup(HtmlElement<T, Attributes...> const& element)
        {
            auto setSideEffect = [self = this](auto const& attribute) {
                auto weak = self->weak_from_this();
                attribute.createEvent(
                    weak,
                    [name = attribute.name()](
                        std::shared_ptr<std::decay_t<decltype(*this)>> const& shared, auto const& value) {
                        shared->setAttribute(name, value);
                    });
            };

            tupleForEach(element.attributes(), [this, &setSideEffect](auto const& attribute) {
                setAttribute(attribute.name(), attribute.value());
                setSideEffect(attribute);
            });
        }

        emscripten::val const& val() const
        {
            return element_;
        }
        emscripten::val& val()
        {
            return element_;
        }
        operator emscripten::val const&() const
        {
            return element_;
        }
        operator emscripten::val&()
        {
            return element_;
        }
        operator emscripten::val&&() &&
        {
            return std::move(element_);
        }

        // TODO: more overloads?
        void setAttribute(std::string_view key, std::string_view value)
        {
            // FIXME: performance fix: val(string(...))
            element_.call<emscripten::val>(
                "setAttribute", emscripten::val{std::string{key}}, emscripten::val{std::string{value}});
        }
        void setAttribute(std::string_view key, std::invocable<emscripten::val> auto&& value)
        {
            element_.set(emscripten::val{std::string{key}}, Nui::bind(value, std::placeholders::_1));
        }

      protected:
        template <typename U, typename... Attributes>
        void replaceElement(HtmlElement<U, Attributes...> const& element)
        {
            auto replacement = BasicElement::createElement(element);
            replacement.setup(element);
            element_.call<emscripten::val>("replaceWith", replacement.val());
            element_ = std::move(replacement).val();
        }

      protected:
        emscripten::val element_;
    };
};
