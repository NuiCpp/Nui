#pragma once

#include <nui/frontend/dom/basic_element.hpp>

#include <concepts>
#include <functional>
#include <memory>
#include <type_traits>

namespace Nui::Dom
{
    template <typename T>
    struct ReferencePasser
    {
      public:
        ReferencePasser(T&& func)
            : func_{std::move(func)}
        {}
        void operator()(std::weak_ptr<Dom::BasicElement>&& weakElement) const
        {
            func_(std::move(weakElement));
        }

      private:
        std::function<void(std::weak_ptr<Dom::BasicElement>&&)> func_;
    };

    namespace Detail
    {
        template <typename T>
        struct IsReferencePasser : std::false_type
        {};
        template <typename T>
        struct IsReferencePasser<ReferencePasser<T>> : std::true_type
        {};
    }

    template <typename T>
    concept IsNotReferencePasser = !Detail::IsReferencePasser<T>::value;
    template <typename T>
    concept IsReferencePasser = Detail::IsReferencePasser<T>::value;

    template <typename T>
    requires std::invocable<T, std::weak_ptr<Dom::BasicElement> &&> auto reference(T&& func)
    {
        return ReferencePasser<T>{std::forward<T>(func)};
    }
}