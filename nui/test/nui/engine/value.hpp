#pragma once

#include "reference_type.hpp"

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

        template <typename... T>
        friend ReferenceType createValue(T&&... ctorArgs);

      private:
        Value()
            : type_{Type::Undefined}
            , value_{std::any{}}
            , isInteger_{false}
        {}
        Value(std::nullptr_t)
            : type_{Type::Null}
            , value_{std::any{}}
            , isInteger_{false}
        {}
        Value(bool value)
            : type_{Type::Boolean}
            , value_{std::any{value}}
            , isInteger_{false}
        {}
        Value(std::floating_point auto value)
            : type_{Type::Number}
            , value_{std::any{static_cast<long double>(value)}}
            , isInteger_{false}
        {}
        Value(std::integral auto value)
            : type_{Type::Number}
            , value_{std::any{static_cast<long long>(value)}}
            , isInteger_{true}
        {}
        Value(std::string_view value)
            : type_{Type::String}
            , value_{std::any{std::string{value}}}
            , isInteger_{false}
        {}
        Value(std::string value)
            : type_{Type::String}
            , value_{std::any{std::move(value)}}
            , isInteger_{false}
        {}

        Value(Object const& value);
        Value(Array const& value);
        Value(Function const& value);

        Value(const Value&) = delete;
        Value& operator=(const Value&) = delete;

      public:
        Value(Value&&) = default;
        Value& operator=(Value&&) = default;

        Value& operator=(std::nullptr_t)
        {
            type_ = Type::Null;
            value_ = std::any{};
            isInteger_ = false;
            return *this;
        }
        Value& operator=(bool value)
        {
            type_ = Type::Boolean;
            value_ = std::any{value};
            isInteger_ = false;
            return *this;
        }
        Value& operator=(std::floating_point auto value)
        {
            type_ = Type::Number;
            value_ = std::any{static_cast<long double>(value)};
            isInteger_ = false;
            return *this;
        }
        Value& operator=(std::integral auto value)
        {
            type_ = Type::Number;
            value_ = std::any{static_cast<long long>(value)};
            isInteger_ = true;
            return *this;
        }
        Value& operator=(std::string_view value)
        {
            type_ = Type::String;
            value_ = std::any{std::string{value}};
            isInteger_ = false;
            return *this;
        }
        Value& operator=(std::string value)
        {
            type_ = Type::String;
            value_ = std::any{std::move(value)};
            isInteger_ = false;
            return *this;
        }
        Value& operator=(Object const& value);
        Value& operator=(Array const& value);
        Value& operator=(Function const& value);

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
            if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T>)
            {
                if (isInteger_ && type_ == Type::Number)
                    return std::any_cast<T>(std::any_cast<long long>(value_));
            }
            return std::any_cast<T>(value_);
        }

        Value const& operator[](std::string_view key) const;
        Value const& operator[](std::size_t index) const;
        Value& operator[](std::string_view key);
        Value& operator[](std::size_t index);
        std::shared_ptr<ReferenceType> reference(std::string_view key);

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

        ReferenceType instanceCounter() const
        {
            return instanceCounter_;
        }

      private:
        Type type_;
        std::any value_;
        bool isInteger_;
        ReferenceType instanceCounter_;
    };
}