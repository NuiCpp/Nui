#pragma once

#include <cstddef>

namespace Nui
{
    struct StringLiteral
    {
        const char* c_str;

        template <std::size_t N>
        // NOLINTNEXTLINE(hicpp-explicit-conversions, cppcoreguidelines-avoid-c-arrays): Intentional
        consteval StringLiteral(const char (&literal)[N])
            : c_str(literal)
        {
            if (literal[N - 1] != '\0')
            {
                // NOLINTBEGIN(hicpp-exception-baseclass): Its a compile time error and my std::exception ctor is not
                // constexpr for some reason.
                throw "not null terminated";
                // NOLINTEND(hicpp-exception-baseclass)
            }
        }
    };
}