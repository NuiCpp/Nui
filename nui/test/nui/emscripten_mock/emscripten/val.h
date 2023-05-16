#pragma once

#include "../../engine/global_object.hpp"
#include "../../engine/array.hpp"
#include "../../engine/object.hpp"
#include "../../engine/function.hpp"
#include "../../engine/value.hpp"
#include "../../engine/warn.hpp"

#include <utility>
#include <type_traits>
#include <vector>
#include <variant>
#include <string>

// REMOVE_ME
#include <iostream>

namespace emscripten
{
    class val
    {
      private:
        auto withValueDo(auto&& fn)
        {
            return std::visit(
                [fn = std::forward<decltype(fn)>(fn)](auto& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Nui::Tests::Engine::Value>)
                    {
                        return fn(value);
                    }
                    else
                    {
                        auto shared = value.lock();
                        if (!shared)
                            throw std::runtime_error{"reference to dead value"};
                        return fn(*shared);
                    }
                },
                referenced_value_);
        }
        auto withValueDo(auto&& fn) const
        {
            return std::visit(
                [fn = std::forward<decltype(fn)>(fn)](auto const& value) {
                    if constexpr (std::is_same_v<std::decay_t<decltype(value)>, Nui::Tests::Engine::Value>)
                    {
                        return fn(value);
                    }
                    else
                    {
                        auto shared = value.lock();
                        if (!shared)
                            throw std::runtime_error{"reference to dead value"};
                        return fn(*shared);
                    }
                },
                referenced_value_);
        }

      public:
        template <typename T>
        val(T value)
            : referenced_value_{Nui::Tests::Engine::Value{std::move(value)}}
        {}
        template <typename T>
        requires Nui::Tests::Engine::Callable<T>
        val(T value)
            : referenced_value_{Nui::Tests::Engine::Function{std::move(value)}}
        {}
        val(std::weak_ptr<Nui::Tests::Engine::Value> value)
            : referenced_value_{std::move(value)}
        {}
        val(Nui::Tests::Engine::Value value)
            : referenced_value_{std::move(value)}
        {}
        val(char const* value)
            : referenced_value_{Nui::Tests::Engine::Value{std::string{value}}}
        {}
        val(val const& other)
            : referenced_value_{other.referenced_value_}
        {}
        val(val&& other) noexcept
            : referenced_value_{std::move(other.referenced_value_)}
        {}
        val()
            : referenced_value_{Nui::Tests::Engine::Value{nullptr}}
        {}

        val& operator=(val const& other)
        {
            referenced_value_ = other.referenced_value_;
            return *this;
        }

        val& operator=(val&& other) noexcept
        {
            referenced_value_ = std::move(other.referenced_value_);
            return *this;
        }

        val operator[](char const* key)
        {
            auto fn = [key](Nui::Tests::Engine::Value& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    return value.template as<Nui::Tests::Engine::Object&>().reference(key);
                else
                    throw std::runtime_error{"val::operator[]: value is not an object"};
            };
            return withValueDo(fn);
        }

        val operator[](char const* key) const
        {
            auto fn = [key](Nui::Tests::Engine::Value const& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    return value.template as<Nui::Tests::Engine::Object const&>()[key];
                else
                    throw std::runtime_error{"val::operator[]: value is not an object"};
            };
            return withValueDo(fn);
        }

        val operator[](int index)
        {
            return withValueDo([index](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Array)
                    return value.template as<Nui::Tests::Engine::Array&>().reference(index);
                else
                    throw std::runtime_error{"val::operator[]: value is not an array"};
            });
        }

        val operator[](int index) const
        {
            return withValueDo([index](auto const& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Array)
                    return value.template as<Nui::Tests::Engine::Array const&>()[index];
                else
                    throw std::runtime_error{"val::operator[]: value is not an array"};
            });
        }

        val operator[](val other)
        {
            if (other.isString())
                return (*this)[other.as<std::string>()];
            else if (other.isNumber())
                return (*this)[other.as<int>()];
            else
                throw std::runtime_error{"val::operator[]: invalid index type"};
        }

        val operator[](val other) const
        {
            if (other.isString())
                return (*this)[other.as<std::string>()];
            else if (other.isNumber())
                return (*this)[other.as<int>()];
            else
                throw std::runtime_error{"val::operator[]: invalid index type"};
        }

        template <typename Ret, typename... List>
        Ret call(char const* name, List&&... args)
        {
            using namespace std::string_literals;

            return withValueDo([name, ... args = std::forward<List>(args)](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                {
                    auto& mem = value.template as<Nui::Tests::Engine::Object&>()[name];
                    if (mem.type() == Nui::Tests::Engine::Value::Type::Function)
                    {
                        if constexpr (std::is_same_v<Ret, void>)
                            return static_cast<void>(mem.template as<Nui::Tests::Engine::Function&>()(args...));
                        else
                            return mem.template as<Nui::Tests::Engine::Function&>()(args...).template as<Ret>();
                    }
                    else
                        throw std::runtime_error{"val::call of "s + name + ": " + mem.typeOf() + " is not a function"};
                }
                else
                    throw std::runtime_error{"val::call: value is not an object"};
            });
        }

        static val global(char const* name)
        {
            std::cout << Nui::Tests::Engine::globalObject.reference(name).lock()->typeOf() << "\n";
            return Nui::Tests::Engine::globalObject.reference(name);
        }

        static val array()
        {
            return val{Nui::Tests::Engine::Array{}};
        }

        static val object()
        {
            using namespace std::string_literals;
            static auto counter = 0;
            return Nui::Tests::Engine::unreferencedObjects.emplace_back(
                "_"s + std::to_string(counter++), Nui::Tests::Engine::Object{});
        }

        static val u8string()
        {
            return val{Nui::Tests::Engine::Value{std::string{}}};
        }

        static val u16string()
        {
            return val{Nui::Tests::Engine::Value{std::string{}}};
        }

        static val undefined()
        {
            return {};
        }

        static val null()
        {
            return val{Nui::Tests::Engine::Value{nullptr}};
        }

        val module_property(char const* key)
        {
            return Nui::Tests::Engine::moduleObject.reference(key);
        }

        bool hasOwnProperty(char const* key) const
        {
            return withValueDo([key](auto const& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    return value.template as<Nui::Tests::Engine::Object const&>().has(key);
                else
                    throw std::runtime_error{"val::hasOwnProperty: value is not an object"};
            });
        }

        template <typename... List>
        val new_(List&&... args)
        {
            return withValueDo([... args = std::forward<List>(args)](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                {
                    auto& obj = value.template as<Nui::Tests::Engine::Object&>();
                    if (obj.has("constructor"))
                    {
                        return obj["constructor"].template as<Nui::Tests::Engine::Function&>()(
                            std::forward<List>(args)...);
                    }
                    else
                    {
                        Nui::Tests::Engine::warn("val::new_: object has no constructor");
                        return Nui::Tests::Engine::Object{};
                    }
                }
                else
                    throw std::runtime_error{"val::new_: value is not an object"};
            });
        }
        void delete_(std::string const& key)
        {
            withValueDo([&key](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    value.template as<Nui::Tests::Engine::Object&>().erase(key);
                else
                    throw std::runtime_error{"val::delete_: value is not an object"};
            });
        };

        void set(char const* key, val const& val)
        {
            withValueDo([&key, &val](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                {
                    auto& object = value.template as<Nui::Tests::Engine::Object&>();
                    val.withValueDo([&object, &key](auto& otherValue) {
                        object[key] = otherValue;
                    });
                }
                else
                    throw std::runtime_error{"val::set: value is not an object"};
            });
        }
        void set(val keyVal, val const& val)
        {
            std::string key;
            keyVal.withValueDo([&keyVal, &val, &key](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::String)
                    key = value.template as<std::string>();
                else
                    throw std::runtime_error{"val::set: key is not a string"};
            });

            set(key.c_str(), val);
        }

        template <typename... List>
        val operator()(List&&... args)
        {
            return withValueDo([... args = std::forward<List>(args)](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Function)
                    return value.template as<Nui::Tests::Engine::Function&>()(args...);
                else
                    throw std::runtime_error{"val::operator(): value is not a function"};
            });
        }

        template <typename T>
        T as() const
        {
            return withValueDo([](auto& value) {
                return value.template as<T>();
            });
        }

        bool isNull() const
        {
            return withValueDo([](auto& value) {
                return value.type() == Nui::Tests::Engine::Value::Type::Null;
            });
        }

        bool isUndefined() const
        {
            return withValueDo([](auto& value) {
                return value.type() == Nui::Tests::Engine::Value::Type::Undefined;
            });
        }

        bool isString() const
        {
            return withValueDo([](auto& value) {
                return value.type() == Nui::Tests::Engine::Value::Type::String;
            });
        }

        bool isNumber() const
        {
            return withValueDo([](auto& value) {
                return value.type() == Nui::Tests::Engine::Value::Type::Number;
            });
        }

        val typeOf() const
        {
            return withValueDo([](auto& value) {
                return val{value.typeOf()};
            });
        }

        val await() const
        {
            return {};
        }

        std::weak_ptr<Nui::Tests::Engine::Value> handle() const
        {
            if (std::holds_alternative<std::weak_ptr<Nui::Tests::Engine::Value>>(referenced_value_))
                return std::get<std::weak_ptr<Nui::Tests::Engine::Value>>(referenced_value_);
            else
                throw std::runtime_error{"val::handle: value is not a reference"};
        }

      private:
        template <typename T>
        friend std::vector<T> vecFromJSArray(val const& v);

        template <typename T>
        friend std::vector<T> convertJSArrayToNumberVector(val const& v);

      private:
        std::variant<std::weak_ptr<Nui::Tests::Engine::Value>, Nui::Tests::Engine::Value> referenced_value_;
    };

    template <typename T>
    std::vector<T> vecFromJSArray(val const& v)
    {
        return v.withValueDo([](auto const& value) {
            if (value.type() == Nui::Tests::Engine::Value::Type::Array)
            {
                auto const& array = value.template as<Nui::Tests::Engine::Array const&>();
                std::vector<T> result;
                result.reserve(array.size());
                for (auto& item : array)
                    result.push_back(item->template as<T>());
                return result;
            }
            else
                throw std::runtime_error{"val::vecFromJSArray: value is not an array"};
        });
    }

    template <typename T>
    std::vector<T> convertJSArrayToNumberVector(val const& v)
    {
        return vecFromJSArray<T>(v);
    }
}

#include "../../engine/function.tpp"