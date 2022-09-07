#pragma once

#include <nui/utility/meta/integral.hpp>

namespace Nui::Detail
{
    template <
        typename Condition,
        template <typename...>
        typename Template,
        template <typename>
        typename Qualification,
        typename Alternative,
        typename... Args>
    struct LazyInstantiate
    {};

    template <
        template <typename...>
        typename Template,
        template <typename>
        typename Qualification,
        typename Alternative,
        typename... Args>
    struct LazyInstantiate<true_, Template, Qualification, Alternative, Args...>
    {
        using type = typename Qualification<Template<Args...>>::type;
    };
    template <
        template <typename...>
        typename Template,
        template <typename>
        typename Qualification,
        typename Alternative,
        typename... Args>
    struct LazyInstantiate<false_, Template, Qualification, Alternative, Args...>
    {
        using type = Alternative;
    };
}