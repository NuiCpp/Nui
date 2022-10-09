#pragma once

#include <tuple>

namespace Nui
{
    template <typename T>
    struct IsTuple
    {
        static constexpr bool value = false;
    };
    template <typename... Ts>
    struct IsTuple<std::tuple<Ts...>>
    {
        static constexpr bool value = true;
    };
    template <typename T>
    constexpr bool IsTuple_v = IsTuple<T>::value;
}