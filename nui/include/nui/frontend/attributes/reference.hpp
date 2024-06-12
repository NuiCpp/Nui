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

        template <typename T>
        requires std::invocable<T, Nui::val&&>
        Attribute onMaterialize(T&& func) const
        {
            return operator=([func = std::forward<T>(func)](std::weak_ptr<Dom::BasicElement>&& element) {
                func(element.lock()->val());
            });
        }

        Attribute operator=(std::weak_ptr<Dom::BasicElement>& ref) const
        {
            return operator=([&ref](std::weak_ptr<Dom::BasicElement>&& element) {
                ref = std::move(element);
            });
        }

        Attribute operator=(Nui::val& ref) const
        {
            return operator=([&ref](std::weak_ptr<Dom::BasicElement>&& element) {
                ref = element.lock()->val();
            });
        }
    } static constexpr reference;
}