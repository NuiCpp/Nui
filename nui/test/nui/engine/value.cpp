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

    Value const& Value::operator[](std::string_view key) const
    {
        if (type() != Type::Object)
            throw std::runtime_error{"operator[]: invalid value type"};
        return as<Object const&>()[key];
    }

    Value const& Value::operator[](std::size_t index) const
    {
        if (type() != Type::Array)
            throw std::runtime_error{"operator[]: invalid value type"};
        return as<Array const&>()[index];
    }

    Value& Value::operator[](std::string_view key)
    {
        if (type() != Type::Object)
            throw std::runtime_error{"operator[]: invalid value type"};
        return as<Object&>()[key];
    }

    Value& Value::operator[](std::size_t index)
    {
        if (type() != Type::Array)
            throw std::runtime_error{"operator[]: invalid value type"};
        return as<Array&>()[index];
    }

    std::weak_ptr<Value> Value::reference(std::string_view key)
    {
        if (type() != Type::Object)
            throw std::runtime_error{"reference: invalid value type"};
        return as<Object&>().reference(key);
    }
}