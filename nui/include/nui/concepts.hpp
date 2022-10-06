#pragma once

#include <concepts>

namespace Nui
{
    template <typename T>
    concept Numerical = std::integral<T> || std::floating_point<T>;

    template <typename T>
    concept Fundamental = Numerical<T> || std::same_as<T, bool>;

    template <typename T>
    concept Incrementable = requires(T t)
    {
        ++t;
        t++;
    };

    template <typename T>
    concept Decrementable = requires(T t)
    {
        --t;
        t--;
    };

    template <typename T, typename U>
    concept PlusAssignable = requires(T t, U u)
    {
        t += u;
    };

    template <typename T, typename U>
    concept MinusAssignable = requires(T t, U u)
    {
        t += u;
    };

    template <typename T, typename U>
    concept InvocableReturns = requires(T func)
    {
        {
            func()
            } -> std::same_as<U>;
    };
}