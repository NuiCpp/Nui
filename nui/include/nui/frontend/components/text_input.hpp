#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/generator_typedefs.hpp>

// elements
#include <nui/frontend/elements/input.hpp>

// attributes
#include <nui/frontend/attributes/type.hpp>
#include <nui/frontend/attributes/value.hpp>
#include <nui/frontend/attributes/on_input.hpp>

namespace Nui::Components
{
    /**
     * @brief This component can be used like this TextInput(attributes...)(children...) instead of the C{}() syntax.
     *
     * @param model The value model (Observed<T>).
     * @param attributes Other attributes to forward.
     */
    template <typename T, typename... Attributes>
    constexpr auto TextInput(Observed<T>& model, Attributes&&... attributes)
    {
        return [&model, ... attributes = std::forward<Attributes>(attributes)]<typename... Children>(
                   Children&&... children) mutable {
            using Nui::Elements::input;
            namespace attr = Nui::Attributes;
            using attr::type;
            using attr::value;
            using attr::onInput;

            return input{
                std::move(attributes)...,
                type = "text",
                value = model,
                onInput =
                    [&model](auto const& event) {
                        model = event["target"]["value"].template as<std::string>();
                    },
            }(std::forward<Children>(children)...);
        };
    }
}