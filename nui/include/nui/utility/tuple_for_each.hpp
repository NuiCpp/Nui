#pragma once

#include <tuple>
#include <type_traits>

namespace Nui
{
    namespace Detail
    {
        template <typename T, typename FuncT, int... Is>
        void tupleForEachImpl(T&& t, FuncT&& func, std::integer_sequence<int, Is...>)
        {
            (..., func(std::get<Is>(t)));
        };
    }

    template <typename FuncT, typename... Types>
    void tupleForEach(std::tuple<Types...>& t, FuncT&& func)
    {
        Detail::tupleForEachImpl(t, std::forward<FuncT>(func), std::make_integer_sequence<int, sizeof...(Types)>());
    }

    template <typename FuncT, typename... Types>
    void tupleForEach(std::tuple<Types...> const& t, FuncT&& func)
    {
        Detail::tupleForEachImpl(t, std::forward<FuncT>(func), std::make_integer_sequence<int, sizeof...(Types)>());
    }
}