#pragma once

#include <tuple>
#include <type_traits>

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
    constexpr bool IsTuple_v = IsTuple<std::decay_t<T>>::value;
}