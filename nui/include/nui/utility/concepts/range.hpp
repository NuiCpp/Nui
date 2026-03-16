#pragma once

#include <concepts>

namespace Nui
{
    template <typename T>
    concept Range = requires(T t) {
        t.begin();
        t.end();
        t.cbegin();
        t.cend();
    };
}