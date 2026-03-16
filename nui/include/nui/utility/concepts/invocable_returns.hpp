#pragma once

#include <concepts>

namespace Nui
{
    template <typename T, typename U>
    concept InvocableReturns = requires(T func) {
        { func() } -> std::same_as<U>;
    };
}