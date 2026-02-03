#pragma once

#include "../../engine/global_object.hpp"
#include "../../engine/array.hpp"
#include "../../engine/object.hpp"
#include "../../engine/function.hpp"
#include "../../engine/value.hpp"
#include "../../engine/warn.hpp"
#include "../../engine/reference_type.hpp"

#include <utility>
#include <type_traits>
#include <vector>
#include <variant>
#include <string>
#include <iostream>

// #define NUI_TEST_DEBUG_PRINT

namespace emscripten
{
    class val
    {
      public:
        struct iterator;

        auto withValueDo(auto&& fn) -> decltype(auto)
        {
            return fn(Nui::Tests::Engine::allValues[*referenced_value_]);
        }
        auto withValueDo(auto&& fn) const -> decltype(auto)
        {
            return fn(Nui::Tests::Engine::allValues[*referenced_value_]);
        }
        auto withValueDo(auto&& fn) const&&
        {
            return fn(Nui::Tests::Engine::allValues[*referenced_value_]);
        }

      public:
        template <typename T>
        val(T value)
            : referenced_value_{std::make_shared<Nui::Tests::Engine::ReferenceType>(
                  Nui::Tests::Engine::createValue(std::move(value)))}
        {}
        template <typename T>
        requires Nui::Tests::Engine::Callable<T> val(T value)
            : referenced_value_{std::make_shared<Nui::Tests::Engine::ReferenceType>(
                  Nui::Tests::Engine::createValue(Nui::Tests::Engine::Function{std::move(value)}))}
        {}
        val(Nui::Tests::Engine::ReferenceType value)
            : referenced_value_{std::make_shared<Nui::Tests::Engine::ReferenceType>(value)}
        {}
        val(std::shared_ptr<Nui::Tests::Engine::ReferenceType> value)
            : referenced_value_{std::move(value)}
        {}
        val(Nui::Tests::Engine::Value const& value)
            : referenced_value_{std::make_shared<Nui::Tests::Engine::ReferenceType>(value.instanceCounter())}
        {}
        val(char const* value)
            : referenced_value_{std::make_shared<Nui::Tests::Engine::ReferenceType>(
                  Nui::Tests::Engine::createValue(std::string{value}))}
        {}
        val(val const& other)
            : referenced_value_{other.referenced_value_}
        {}
        val(val&& other) noexcept
            : referenced_value_{std::move(other.referenced_value_)}
        {}
        val()
            : referenced_value_{std::make_shared<Nui::Tests::Engine::ReferenceType>(-1)}
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

        template <typename T>
        auto as() const& -> decltype(auto)
        {
            if constexpr (std::is_same_v<T, val>)
                return *this;
            else
                return Nui::Tests::Engine::allValues[*referenced_value_].template as<T const&>();
        }

        template <typename T>
        auto as() & -> decltype(auto)
        {
            if constexpr (std::is_same_v<T, val>)
                return *this;
            else
                return withValueDo([](auto& value) -> decltype(auto) {
                    return value.template as<T&>();
                });
        }

        template <typename T>
        T as() &&
        {
            if constexpr (std::is_same_v<T, val>)
                return *this;
            else
                return withValueDo([](auto&& value) -> T {
                    return value.template as<T>();
                });
        }

        val operator[](char const* key)
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::operator[" << key << "]\n";
#endif
            auto fn = [key](Nui::Tests::Engine::Value& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    return value.template as<Nui::Tests::Engine::Object&>().reference(key);
                else if (value.type() == Nui::Tests::Engine::Value::Type::Array)
                    return value.template as<Nui::Tests::Engine::Array&>().asObject().reference(key);
                else if (value.type() == Nui::Tests::Engine::Value::Type::Function)
                {
                    if (std::string{key} == "opcall")
                        return std::make_shared<Nui::Tests::Engine::ReferenceType>(value.instanceCounter());
                    else
                        throw std::runtime_error{"val::operator[]: function has no member " + std::string{key}};
                }
                else
                    throw std::runtime_error{"val::operator[]: value is not an object"};
            };
            return withValueDo(fn);
        }

        val operator[](char const* key) const
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::operator[" << key << "]\n";
#endif
            auto fn = [key](Nui::Tests::Engine::Value const& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    return value.template as<Nui::Tests::Engine::Object const&>().reference(key);
                else
                    throw std::runtime_error{"val::operator[]: value is not an object"};
            };
            return withValueDo(fn);
        }

        val operator[](int index)
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::operator[" << index << "]\n";
#endif
            return withValueDo([index](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Array)
                    return value.template as<Nui::Tests::Engine::Array&>().reference(index);
                else
                    throw std::runtime_error{"val::operator[]: value is not an array"};
            });
        }

        val operator[](int index) const
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::operator[" << index << "]\n";
#endif
            return withValueDo([index](auto const& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Array)
                    return value.template as<Nui::Tests::Engine::Array const&>().reference(index);
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
        Ret call(char const* name, List&&... args) const
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::call<" << boost::typeindex::type_id<Ret>().pretty_name() << "("
                      << Nui::Tests::Engine::Detail::TupleTypePrint<std::tuple<List...>>::toString() << ")>: " << name
                      << "\n";
#endif
            using namespace std::string_literals;

            return withValueDo([this, name, ... args = std::forward<List>(args)](auto& value) {
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
                else if (value.type() == Nui::Tests::Engine::Value::Type::Function)
                {
                    // so far only used to bind this correctly for functions.
                    // ignore for now.
                    if (std::string{name} == "bind")
                    {
                        if constexpr (std::is_same_v<Ret, void>)
                            return;
                        else
                            return val{value.instanceCounter()};
                    }
                    else
                        throw std::runtime_error{"val::call of "s + name + ": function has no member " + name};
                }
                else
                    throw std::runtime_error{
                        "val::call: value is not an object, value is '"s + typeOf().as<std::string>() + "'"};
            });
        }

        static val global(char const* name)
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::global(" << name << ")\n";
            std::cout << "type: "
                      << Nui::Tests::Engine::allValues[*Nui::Tests::Engine::globalObject.reference(name)].typeOf()
                      << "\n";
#endif
            return Nui::Tests::Engine::globalObject.reference(name);
        }

        static val array()
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::array()\n";
#endif
            return Nui::Tests::Engine::createValue(Nui::Tests::Engine::Array{});
        }

        static val object()
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::object()\n";
#endif
            using namespace std::string_literals;
            static auto counter = 0;
            return Nui::Tests::Engine::createValue(Nui::Tests::Engine::Object{});
        }

        static val u8string()
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::u8string()\n";
#endif

            return Nui::Tests::Engine::createValue(std::string{});
        }

        static val u16string()
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::u16string()\n";
#endif
            return Nui::Tests::Engine::createValue(std::string{});
        }

        static val undefined()
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::undefined()\n";
#endif
            return Nui::Tests::Engine::createValue();
        }

        static val null()
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::null()\n";
#endif
            return Nui::Tests::Engine::createValue(nullptr);
        }

        val module_property(char const* key)
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::module_property(" << key << ")\n";
#endif
            return Nui::Tests::Engine::moduleObject.reference(key);
        }

        bool hasOwnProperty(char const* key) const
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::hasOwnProperty(" << key << ")\n";
#endif
            using namespace std::string_literals;

            return withValueDo([this, key](auto const& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    return value.template as<Nui::Tests::Engine::Object const&>().has(key);
                else
                    throw std::runtime_error{
                        "val::hasOwnProperty: value is not an object, value is '"s + typeOf().as<std::string>() + "'"};
            });
        }

        template <typename... List>
        val new_(List&&... args)
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::new_()\n";
#endif
            return withValueDo([... args = std::forward<List>(args)](auto& value) -> val {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                {
                    auto& obj = value.template as<Nui::Tests::Engine::Object&>();
                    if (obj.has("constructor"))
                    {
                        return obj["constructor"].template as<Nui::Tests::Engine::Function&>()(args...);
                    }
                    Nui::Tests::Engine::warn("val::new_: object has no constructor");
                    return Nui::Tests::Engine::Object{};
                }
                throw std::runtime_error{"val::new_: value is not an object"};
            });
        }
        void delete_(std::string const& key)
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::delete_(" << key << ")\n";
#endif
            withValueDo([&key](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                    value.template as<Nui::Tests::Engine::Object&>().erase(key);
                else
                    throw std::runtime_error{"val::delete_: value is not an object"};
            });
        };

        void set(char const* key, val const& val) const
        {
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::set(" << key << ", " << val.typeOf().template as<std::string>() << ")\n";
#endif
            withValueDo([&key, &val](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Object)
                {
                    auto& object = value.template as<Nui::Tests::Engine::Object&>();
                    object.set(key, val.referenced_value_);
                }
                else
                    throw std::runtime_error{"val::set: value is not an object"};
            });
        }
        void set(val keyVal, val const& val) const
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
#ifdef NUI_TEST_DEBUG_PRINT
            std::cout << "val::operator()()\n";
#endif
            return withValueDo([... args = std::forward<List>(args)](auto& value) {
                if (value.type() == Nui::Tests::Engine::Value::Type::Function)
                    return value.template as<Nui::Tests::Engine::Function&>()(args...);
                else
                    throw std::runtime_error{"val::operator(): value is not a function"};
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

        std::shared_ptr<Nui::Tests::Engine::ReferenceType> handle() const
        {
            return referenced_value_;
        }

        void print() const
        {
            withValueDo([](auto& value) {
                value.print();
            });
        }

        iterator begin() const;
        // our iterators are sentinel-based range iterators; use nullptr as the end sentinel
        constexpr std::nullptr_t end() const
        {
            return nullptr;
        }

      private:
        template <typename T>
        friend std::vector<T> vecFromJSArray(val const& v);

        template <typename T>
        friend std::vector<T> convertJSArrayToNumberVector(val const& v);

      private:
        std::shared_ptr<Nui::Tests::Engine::ReferenceType> referenced_value_;
    };

    struct val::iterator
    {
        iterator() = delete;
        // Make sure iterator is only moveable, not copyable as it represents a mutable state.
        iterator(iterator&&) = default;
        iterator(iterator const&) = delete;
        ~iterator() = default;
        explicit iterator(val v)
            : v_(std::move(v))
            , cur_value_{}
            , index_{0}
        {
            this->operator++();
        }
        iterator& operator=(iterator&&) = default;
        iterator& operator=(const iterator&) = delete;
        val&& operator*()
        {
            return std::move(cur_value_);
        }
        const val& operator*() const
        {
            return cur_value_;
        }
        void operator++()
        {
            cur_value_ = v_[static_cast<int>(index_)];
            ++index_;
        }
        bool operator!=(std::nullptr_t) const
        {
            return cur_value_.handle() != nullptr;
        }

      private:
        val v_;
        val cur_value_;
        std::size_t index_;
    };

    inline val::iterator val::begin() const
    {
        return val::iterator{*this};
    }

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
                    result.push_back(Nui::Tests::Engine::allValues[*item].template as<T>());
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