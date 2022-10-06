#pragma once

namespace Nui::Detail
{
    template <typename T, typename>
    struct PickFirst
    {
        using type = T;
    };

    template <typename T, typename U>
    using PickFirst_t = typename PickFirst<T, U>::type;
}