#include "value.hpp"

#include "object.hpp"
#include "array.hpp"
#include "function.hpp"

namespace Nui::Tests::Engine
{
    Value::Value(Object const& value)
        : type_{Type::Object}
        , value_{std::any{value}}
        , isInteger_{false}
    {}

    Value::Value(Array const& value)
        : type_{Type::Array}
        , value_{std::any{value}}
        , isInteger_{false}
    {}

    Value::Value(Function const& value)
        : type_{Type::Function}
        , value_{std::any{value}}
        , isInteger_{false}
    {}

    Value& Value::operator=(Object const& value)
    {
        type_ = Type::Object;
        value_ = std::any{value};
        return *this;
    }
    Value& Value::operator=(Array const& value)
    {
        type_ = Type::Array;
        value_ = std::any{value};
        return *this;
    }
    Value& Value::operator=(Function const& value)
    {
        type_ = Type::Function;
        value_ = std::any{value};
        return *this;
    }

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

    std::shared_ptr<ReferenceType> Value::reference(std::string_view key)
    {
        if (type() != Type::Object)
            throw std::runtime_error{"reference: invalid value type"};
        return as<Object&>().reference(key);
    }

    void Value::print(int indent, std::vector<ReferenceType> referenceStack) const
    {
        // test stack for circular references
        for (auto const& reference : referenceStack)
        {
            if (reference == instanceCounter_)
            {
                if (type() == Type::Object)
                    std::cout << "{...recursive...}";
                else if (type() == Type::Array)
                    std::cout << "[...recursive...]";
                else
                    std::cout << "...";
                return;
            }
        }
        referenceStack.push_back(ReferenceType{instanceCounter_});

        switch (type())
        {
            case Nui::Tests::Engine::Value::Type::Null:
            {
                std::cout << "null";
                break;
            }
            case Nui::Tests::Engine::Value::Type::Undefined:
            {
                std::cout << "undefined";
                break;
            }
            case Nui::Tests::Engine::Value::Type::Boolean:
            {
                std::cout << std::boolalpha << as<bool>();
                break;
            }
            case Nui::Tests::Engine::Value::Type::Number:
            {
                if (isInteger_)
                    std::cout << as<long long>();
                else
                    std::cout << as<long double>();
                break;
            }
            case Nui::Tests::Engine::Value::Type::String:
            {
                std::cout << as<std::string>();
                break;
            }
            case Nui::Tests::Engine::Value::Type::Object:
            {
                as<Object const&>().print(indent, referenceStack);
                break;
            }
            case Nui::Tests::Engine::Value::Type::Array:
            {
                as<Array const&>().print(indent, referenceStack);
                break;
            }
            case Nui::Tests::Engine::Value::Type::Function:
            {
                as<Function const&>().print(indent);
                break;
            }
            default:
            {
                throw std::runtime_error{"print: invalid value type"};
            }
        }
    }
}