#pragma once

#include <nui/frontend/dom/reference.hpp>

namespace Nui::Attributes
{
    struct reference_
    {
        template <typename T>
        requires std::invocable<T, std::weak_ptr<Dom::BasicElement>&&>
        Attribute operator=(T&& func) const
        {
            return Attribute{[func = std::forward<T>(func)](Dom::ChildlessElement& element) {
                func(element.weak_from_base<Dom::BasicElement>());
            }};
        }

        Attribute operator=(std::weak_ptr<Dom::BasicElement>& ref) const
        {
            return operator=([&ref](std::weak_ptr<Dom::BasicElement>&& element) {
                ref = std::move(element);
            });
        }
    } static constexpr reference;
}