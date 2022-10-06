#pragma once

#include <nui/utility/meta/tuple_push_front.hpp>

#include <type_traits>
#include <tuple>

namespace Nui::Detail
{
    template <template <typename> typename Predicate, typename... T>
    struct TupleFilter
    {
        using type = std::tuple<>;
    };

    template <template <typename> typename Predicate, typename Head, typename... Tail>
    struct TupleFilter<Predicate, Head, Tail...>
    {
      private:
        using previous = typename TupleFilter<Predicate, Tail...>::type;

      public:
        using type = std::conditional_t<Predicate<Head>::value, TuplePushFront_t<previous, Head>, previous>;
    };

    template <template <typename> typename Predicate, typename... T>
    using TupleFilter_t = typename TupleFilter<Predicate, T...>::type;
}