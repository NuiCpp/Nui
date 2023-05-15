#include "value.hpp"

#include "object.hpp"
#include "array.hpp"
#include "function.hpp"

namespace Nui::Tests::Engine
{
    Value::Value(Object const& value)
        : type_{Type::Object}
        , value_{std::any{value}}
    {}

    Value::Value(Array const& value)
        : type_{Type::Array}
        , value_{std::any{value}}
    {}

    Value::Value(Function const& value)
        : type_{Type::Function}
        , value_{std::any{value}}
    {}
}