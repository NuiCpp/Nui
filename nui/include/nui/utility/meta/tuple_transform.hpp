#pragma once

#include <tuple>

namespace Nui::Detail
{
    template <typename Tuple, template <typename> typename Predicate>
    struct TupleTransform
    {};

    template <template <typename> typename Predicate, typename... List>
    struct TupleTransform<std::tuple<List...>, Predicate>
    {
        using type = std::tuple<typename Predicate<List>::type...>;
    };

    template <typename Tuple, template <typename> typename Predicate>
    using TupleTransform_t = typename TupleTransform<Tuple, Predicate>::type;
}