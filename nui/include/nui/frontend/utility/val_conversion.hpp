#pragma once

#include <nui/concepts.hpp>
#include <nui/frontend/event_system/observed_value.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wold-style-cast"
#include <boost/describe.hpp>
#include <boost/mp11/algorithm.hpp>
#pragma clang diagnostic pop

#include <nui/frontend/val.hpp>

#include <optional>
#include <vector>
#include <filesystem>
#include <utility>
#include <unordered_map>

namespace Nui
{
    namespace Detail
    {
        template <typename T>
        struct IsOptional : std::false_type
        {};
        template <typename T>
        struct IsOptional<std::optional<T>> : std::true_type
        {};
    }

    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union<T>::value>>
    void convertToVal(Nui::val& val, T const& obj);
    template <typename T>
    Nui::val convertToVal(std::optional<T> const& option);
    template <typename T>
    requires Fundamental<T>
    Nui::val convertToVal(T const& value);
    Nui::val convertToVal(std::string const& value);
    Nui::val convertToVal(std::filesystem::path const& value);
    Nui::val convertToVal(Nui::val value);
    Nui::val convertToVal(char const* value);
    template <typename T>
    Nui::val convertToVal(std::vector<T> const& vector);
    template <typename T>
    Nui::val convertToVal(Observed<T> const& observed);
    template <typename T>
    Nui::val convertToVal(std::unordered_map<std::string, T> const& map);
    inline Nui::val convertToVal(long long)
    {
        throw std::runtime_error("Cannot convert long long to val");
    }

    template <typename T, class Bases, class Members, class Enable>
    void convertFromVal(Nui::val const& val, T& obj);
    template <typename T>
    requires Fundamental<T>
    void convertFromVal(Nui::val const& val, T& value);
    void convertFromVal(Nui::val const& val, std::string& str);
    template <typename T>
    void convertFromVal(Nui::val const& val, std::optional<T>& option);
    void convertFromVal(Nui::val const& val, std::filesystem::path& value);
    void convertFromVal(Nui::val const& val, Nui::val& value);
    template <typename T>
    void convertFromVal(Nui::val const& val, std::vector<T>& vector);
    template <typename T>
    requires Fundamental<T>
    void convertFromVal(Nui::val const& val, std::vector<T>& vector);
    template <typename T>
    void convertFromVal(Nui::val const& val, Observed<T>& observed);
    template <typename T>
    void convertFromVal(Nui::val const& val, std::unordered_map<std::string, T>& map);
    inline void convertFromVal(Nui::val const&, long long)
    {
        throw std::invalid_argument("Cannot convert from val to long long");
    }

    template <typename T, class Bases, class Members, class Enable>
    void convertToVal(Nui::val& val, T const& obj)
    {
        if (val.typeOf().as<std::string>() != "object")
            val = Nui::val::object();

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
    Nui::val convertToVal(T const& obj)
    {
        Nui::val val = Nui::val::object();
        convertToVal(val, obj);
        return val;
    }

    template <typename T>
    Nui::val convertToVal(std::optional<T> const& option)
    {
        return option ? convertToVal(*option) : Nui::val::undefined();
    }
    template <typename T>
    requires Fundamental<T>
    Nui::val convertToVal(T const& value)
    {
        return Nui::val{value};
    }
    inline Nui::val convertToVal(std::string const& value)
    {
        return Nui::val{value};
    }
    inline Nui::val convertToVal(std::filesystem::path const& value)
    {
        return Nui::val{value.string()};
    }
    inline Nui::val convertToVal(Nui::val value)
    {
        return value;
    }
    inline Nui::val convertToVal(char const* value)
    {
        return Nui::val{std::string{value}};
    }
    template <typename T>
    Nui::val convertToVal(std::vector<T> const& vector)
    {
        Nui::val result = Nui::val::array();
        for (auto const& element : vector)
            result.call<void>("push", convertToVal(element));
        return result;
    }
    template <typename T>
    Nui::val convertToVal(Observed<T> const& observed)
    {
        return convertToVal(observed.value());
    }
    template <typename T>
    Nui::val convertToVal(std::unordered_map<std::string, T> const& map)
    {
        Nui::val result = Nui::val::object();
        for (auto const& [key, value] : map)
            result.set(key, convertToVal(value));
        return result;
    }

    template <typename T, class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>>
    requires(!std::is_union_v<T>)
    void convertFromValObjImpl(Nui::val const& val, T& obj)
    {
        boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
            if (val.hasOwnProperty(memAccessor.name))
            {
                if constexpr (!Detail::IsOptional<decltype(obj.*memAccessor.pointer)>::value)
                {
                    if (val[memAccessor.name].isNull() || val[memAccessor.name].isUndefined())
                        Nui::val::global("console").call<void>(
                            "error",
                            std::string{"Expected member "} + memAccessor.name + " to be defined and non null");
                    else
                        convertFromVal(val[memAccessor.name], obj.*memAccessor.pointer);
                }
                else
                    convertFromVal(val[memAccessor.name], obj.*memAccessor.pointer);
            }
        });
    }

    template <typename T, class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>>
    requires(!std::is_union_v<T> && !boost::describe::has_describe_bases<T>::value)
    void convertFromVal(Nui::val const& val, T& obj)
    {
        convertFromValObjImpl(val, obj);
    }

    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>>
    requires(!std::is_union_v<T> && boost::describe::has_describe_bases<T>::value)
    void convertFromVal(Nui::val const& val, T& obj)
    {
        boost::mp11::mp_for_each<Bases>([&](auto&& base) {
            using type = typename std::decay_t<decltype(base)>::type;
            convertFromVal(val, static_cast<type&>(obj));
        });
        convertFromValObjImpl(val, obj);
    }

    template <typename T>
    requires Fundamental<T>
    void convertFromVal(Nui::val const& val, T& value)
    {
        value = val.as<T>();
    }
    inline void convertFromVal(Nui::val const& val, std::string& str)
    {
        str = val.as<std::string>();
    }
    template <typename T>
    void convertFromVal(Nui::val const& val, std::optional<T>& option)
    {
        if (val.isNull() || val.isUndefined())
            option = std::nullopt;
        else
        {
            T value;
            convertFromVal(val, value);
            option = value;
        }
    }
    inline void convertFromVal(Nui::val const& val, std::filesystem::path& value)
    {
        value = val.as<std::string>();
    }
    inline void convertFromVal(Nui::val const& val, Nui::val& value)
    {
        value = val;
    }
    template <typename T>
    void convertFromVal(Nui::val const& val, std::vector<T>& vector)
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
    void convertFromVal(Nui::val const& val, std::vector<T>& vector)
    {
        vector = emscripten::convertJSArrayToNumberVector<T>(val);
    }
    template <typename T>
    void convertFromVal(Nui::val const& val, Observed<T>& observed)
    {
        auto proxy = observed.modify();
        convertFromVal(val, proxy.value());
    }
    template <typename T>
    void convertFromVal(Nui::val const& val, std::unordered_map<std::string, T>& map)
    {
        map.clear();
        const auto keys = Nui::val::global("Object").call<Nui::val>("keys", val);
        const auto length = keys["length"].as<std::size_t>();
        for (std::size_t i = 0; i < length; ++i)
        {
            const auto key = keys[i].as<std::string>();
            T value;
            convertFromVal(val[key], value);
            map.emplace(key, std::move(value));
        }
    }
}