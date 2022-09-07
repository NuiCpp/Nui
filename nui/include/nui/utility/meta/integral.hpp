#pragma once

#include <type_traits>
#include <cstdint>

namespace Nui::Detail
{
    template <typename T, T Value>
    using integral = std::integral_constant<T, Value>;

#define MPLEX_DECLARE_INTEGRAL(TYPE) \
    template <TYPE V> \
    using TYPE##_ = Nui::Detail::integral<TYPE, V>

#define MPLEX_DECLARE_INTEGRAL_N(TYPE, NAME) \
    template <TYPE V> \
    using NAME##_ = Nui::Detail::integral<TYPE, V>

    MPLEX_DECLARE_INTEGRAL(bool);
    MPLEX_DECLARE_INTEGRAL(int);
    MPLEX_DECLARE_INTEGRAL(long);
    MPLEX_DECLARE_INTEGRAL_N(long long, long_long);
    MPLEX_DECLARE_INTEGRAL(short);
    MPLEX_DECLARE_INTEGRAL(unsigned);

    MPLEX_DECLARE_INTEGRAL_N(unsigned int, unsigned_int);
    MPLEX_DECLARE_INTEGRAL_N(unsigned long, unsigned_long);
    MPLEX_DECLARE_INTEGRAL_N(unsigned long long, unsigned_long_long);
    MPLEX_DECLARE_INTEGRAL_N(unsigned short, unsigned_short);

    MPLEX_DECLARE_INTEGRAL(uint8_t);
    MPLEX_DECLARE_INTEGRAL(uint16_t);
    MPLEX_DECLARE_INTEGRAL(uint32_t);
    MPLEX_DECLARE_INTEGRAL(uint64_t);

    MPLEX_DECLARE_INTEGRAL(int8_t);
    MPLEX_DECLARE_INTEGRAL(int16_t);
    MPLEX_DECLARE_INTEGRAL(int32_t);
    MPLEX_DECLARE_INTEGRAL(int64_t);

    using true_ = bool_<true>;
    using false_ = bool_<false>;
}