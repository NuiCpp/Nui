#pragma once

#include <concepts>
#include <any>
#include <string>
#include <string_view>
#include <memory>
#include <iostream>

namespace Nui::Tests::Engine
{
    class Object;
    class Array;
    class Function;

    class Value
    {
      public:
        enum class Type
        {
            Undefined,
            Null,
            Boolean,
            Number,
            String,
            Object,
            Array,
            Function
        };

        Value()
            : type_{Type::Undefined}
            , value_{std::any{}}
        {}
        Value(std::nullptr_t)
            : type_{Type::Null}
            , value_{std::any{}}
        {}
        Value(bool value)
            : type_{Type::Boolean}
            , value_{std::any{value}}
        {}

        Value(std::floating_point auto value)
            : type_{Type::Number}
            , value_{std::any{static_cast<long double>(value)}}
        {}

        Value(std::integral auto value)
            : type_{Type::Number}
            , value_{std::any{static_cast<long double>(value)}}
        {}

        Value(std::string_view value)
            : type_{Type::String}
            , value_{std::any{std::string{value}}}
        {}

        Value(std::string value)
            : type_{Type::String}
            , value_{std::any{std::move(value)}}
        {}

        Value(Object const& value);
        Value(Array const& value);
        Value(Function const& value);

        Value(const Value&) = default;
        Value(Value&&) = default;
        Value& operator=(const Value&) = default;
        Value& operator=(Value&&) = default;

        Type type() const
        {
            return type_;
        }

        template <typename T>
        T& as() &
        {
            return std::any_cast<T&>(value_);
        }
        template <typename T>
        T const& as() const&
        {
            return std::any_cast<T const&>(value_);
        }
        template <typename T>
        T as() &&
        {
            return std::any_cast<T>(value_);
        }

        Value const& operator[](std::string_view key) const;
        Value const& operator[](std::size_t index) const;
        Value& operator[](std::string_view key);
        Value& operator[](std::size_t index);
        std::weak_ptr<Value> reference(std::string_view key);

        std::string typeOf() const
        {
            switch (type())
            {
                case Nui::Tests::Engine::Value::Type::Null:
                    return "null";
                case Nui::Tests::Engine::Value::Type::Undefined:
                    return "undefined";
                case Nui::Tests::Engine::Value::Type::Boolean:
                    return "boolean";
                case Nui::Tests::Engine::Value::Type::Number:
                    return "number";
                case Nui::Tests::Engine::Value::Type::String:
                    return "string";
                case Nui::Tests::Engine::Value::Type::Object:
                    return "object";
                case Nui::Tests::Engine::Value::Type::Array:
                    return "array";
                case Nui::Tests::Engine::Value::Type::Function:
                    return "function";
                default:
                    throw std::runtime_error{"typeOf: invalid value type"};
            }
        }

        void print(int indent = 0);

      private:
        Type type_;
        std::any value_;
    };
}