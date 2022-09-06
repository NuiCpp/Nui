#pragma once

#include <concepts>

namespace Nui
{
    template <typename T>
    concept Numerical = std::integral<T> || std::floating_point<T>;

    template <typename T>
    concept Fundamental = Numerical<T> || std::same_as<T, bool>;
}