#pragma once

#include "object.hpp"
#include "reference_type.hpp"

#include <vector>

namespace Nui::Tests::Engine
{
    extern Object globalObject;
    extern Object moduleObject;
    extern ReferenceType valueCounter;
    extern std::vector<Value> allValues;

    void resetGlobals();

    template <typename... T>
    ReferenceType createValue(T&&... ctorArgs)
    {
        Value value{std::forward<T>(ctorArgs)...};
        const auto cnt = valueCounter++;
        value.instanceCounter_ = cnt;
        allValues.emplace_back(std::move(value));
        return cnt;
    }
}