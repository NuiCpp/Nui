#pragma once

#include <nui/concepts.hpp>
#include <nui/frontend/event_system/observed_value.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wold-style-cast"
#include <boost/describe.hpp>
#include <boost/mp11/algorithm.hpp>
#pragma clang diagnostic pop

#include <emscripten/val.h>

#include <optional>
#include <vector>
#include <filesystem>
#include <utility>
#include <unordered_map>

namespace Nui
{
    template <typename T>
    emscripten::val convertToVal(std::optional<T> const& option);
    template <typename T>
    requires Fundamental<T> emscripten::val convertToVal(T const& value);
    emscripten::val convertToVal(std::string const& value);
    emscripten::val convertToVal(std::filesystem::path const& value);
    emscripten::val convertToVal(emscripten::val value);
    emscripten::val convertToVal(char const* value);
    template <typename T>
    emscripten::val convertToVal(std::vector<T> const& vector);
    template <typename T>
    emscripten::val convertToVal(Observed<T> const& observed);

    template <typename T>
    requires Fundamental<T>
    void convertFromVal(emscripten::val const& val, T& value);
    void convertFromVal(emscripten::val const& val, std::string& str);
    template <typename T>
    void convertFromVal(emscripten::val const& val, std::optional<T>& option);
    void convertFromVal(emscripten::val const& val, std::filesystem::path& value);
    void convertFromVal(emscripten::val const& val, emscripten::val& value);
    template <typename T>
    void convertFromVal(emscripten::val const& val, std::vector<T>& vector);
    template <typename T>
    requires Fundamental<T>
    void convertFromVal(emscripten::val const& val, std::vector<T>& vector);
    template <typename T>
    void convertFromVal(emscripten::val const& val, Observed<T>& observed);

    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union<T>::value>>
    void convertToVal(emscripten::val& val, T const& obj)
    {
        if (val.typeOf().as<std::string>() != "object")
            val = emscripten::val::object();

        boost::mp11::mp_for_each<Bases>([&](auto&& base) {
            using type = typename std::decay_t<decltype(base)>::type;
            convertToVal(val, static_cast<type const&>(obj));
        });

        boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
            val.set(memAccessor.name, convertToVal(obj.*memAccessor.pointer));
        });
    }
    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union<T>::value>>
    emscripten::val convertToVal(T const& obj)
    {
        emscripten::val val = emscripten::val::object();
        convertToVal(val, obj);
        return val;
    }

    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union<T>::value>>
    void convertFromVal(emscripten::val const& val, T& obj)
    {
        boost::mp11::mp_for_each<Bases>([&](auto&& base) {
            using type = typename std::decay_t<decltype(base)>::type;
            convertFromVal(val, static_cast<type&>(obj));
        });

        boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
            if (val.hasOwnProperty(memAccessor.name))
                convertFromVal(val[memAccessor.name], obj.*memAccessor.pointer);
        });
    }
    template <typename T>
    emscripten::val convertToVal(std::optional<T> const& option)
    {
        return option ? emscripten::val(*option) : emscripten::val::undefined();
    }
    template <typename T>
    requires Fundamental<T> emscripten::val convertToVal(T const& value)
    {
        return emscripten::val{value};
    }
    inline emscripten::val convertToVal(std::string const& value)
    {
        return emscripten::val{value};
    }
    inline emscripten::val convertToVal(std::filesystem::path const& value)
    {
        return emscripten::val{value.string()};
    }
    inline emscripten::val convertToVal(emscripten::val value)
    {
        return value;
    }
    inline emscripten::val convertToVal(char const* value)
    {
        return emscripten::val{std::string{value}};
    }
    template <typename T>
    emscripten::val convertToVal(std::vector<T> const& vector)
    {
        emscripten::val result = emscripten::val::array();
        for (auto const& element : vector)
            result.call<void>("push", convertToVal(element));
        return result;
    }
    template <typename T>
    emscripten::val convertToVal(Observed<T> const& observed)
    {
        return convertToVal(observed.value());
    }

    template <typename T>
    requires Fundamental<T>
    void convertFromVal(emscripten::val const& val, T& value)
    {
        value = val.as<T>();
    }
    inline void convertFromVal(emscripten::val const& val, std::string& str)
    {
        str = val.as<std::string>();
    }
    template <typename T>
    void convertFromVal(emscripten::val const& val, std::optional<T>& option)
    {
        const auto typeOf = val.typeOf().as<std::string>();
        if (typeOf == "undefined" || typeOf == "null")
            option = std::nullopt;
        else
        {
            T value;
            convertFromVal(val, value);
            option = value;
        }
    }
    inline void convertFromVal(emscripten::val const& val, std::filesystem::path& value)
    {
        value = val.as<std::string>();
    }
    inline void convertFromVal(emscripten::val const& val, emscripten::val& value)
    {
        value = val;
    }
    template <typename T>
    void convertFromVal(emscripten::val const& val, std::vector<T>& vector)
    {
        vector.clear();
        const auto length = val["length"].as<std::size_t>();
        vector.reserve(length);
        for (std::size_t i = 0; i < length; ++i)
        {
            T value;
            convertFromVal(val[i], value);
            vector.push_back(std::move(value));
        }
    }
    template <typename T>
    requires Fundamental<T>
    void convertFromVal(emscripten::val const& val, std::vector<T>& vector)
    {
        vector = emscripten::convertJSArrayToNumberVector<T>(val);
    }
    template <typename T>
    void convertFromVal(emscripten::val const& val, Observed<T>& observed)
    {
        auto proxy = observed.modify();
        convertFromVal(val, proxy.value());
    }
}