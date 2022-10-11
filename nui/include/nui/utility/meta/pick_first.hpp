#pragma once

namespace Nui::Detail
{
    template <typename...>
    struct PickFirst
    {};

    template <typename T, typename... Ts>
    struct PickFirst<T, Ts...>
    {
        using type = T;
    };

    template <>
    struct PickFirst<>
    {
        using type = void;
    };

    template <typename... Ts>
    using PickFirst_t = typename PickFirst<Ts...>::type;
}