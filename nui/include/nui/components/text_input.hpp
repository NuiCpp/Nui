#pragma once

#include <nui/event_system/observed_value.hpp>

// elements
#include <nui/elements/input.hpp>

// attributes
#include <nui/attributes/type.hpp>
#include <nui/attributes/value.hpp>
#include <nui/attributes/input_events.hpp>

#include <tuple>

namespace Nui
{
    template <typename... Args>
    class TextInput
    {
      public:
        constexpr TextInput(Nui::Attribute<Nui::Attributes::valueTag, Observed<std::string>>&& value, Args&&... args)
            : modelText(value.observed())
            , args(std::forward<Args>(args)...)
        {}

        template <typename... Children>
        constexpr auto operator()(Children&&... children) &&
        {
            using Nui::Elements::input;
            namespace attr = Nui::Attributes;
            using namespace attr::Literals;
            using attr::type;
            using attr::value;
            using attr::onInput;

            // clang-format off
            return std::apply([&](auto&&... args)
            {
                return input{
                    std::forward<decltype(args)>(args)...,
                    type = "text",
                    value = modelText,
                    onInput = [&modelText = this->modelText](auto const& event)
                    {
                        modelText = event["target"]["value"].template as<std::string>();
                    }
                }(std::forward<Children>(children)...);
            }, args);
            // clang-format on
        }

      private:
        Observed<std::string>& modelText;
        std::tuple<Args...> args;
    };
}