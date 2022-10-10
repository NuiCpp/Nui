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
        T func_;
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

    auto reference(std::invocable<std::weak_ptr<Dom::BasicElement>&&> auto&& func)
    {
        return ReferencePasser{std::forward<decltype(func)>(func)};
    }
}