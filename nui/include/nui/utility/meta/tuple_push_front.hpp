#pragma once

#include <tuple>

namespace Nui::Detail
{
    template <typename Tuple, typename Elem>
    struct TuplePushFront
    {};

    template <typename Elem, typename... List>
    struct TuplePushFront<std::tuple<List...>, Elem>
    {
        using type = std::tuple<Elem, List...>;
    };

    template <typename Tuple, typename Elem>
    using TuplePushFront_t = typename TuplePushFront<Tuple, Elem>::type;
}