#pragma once

namespace Nui
{
    template <typename T>
    struct ExtractValueType
    {
        using type = typename T::value_type;
    };
    template <typename T>
    using ExtractValueType_t = typename ExtractValueType<T>::type;
}