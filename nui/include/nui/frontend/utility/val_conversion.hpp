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
#include <string_view>
#include <memory>
#include <map>
#include <variant>

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

        /**
         * @brief Concept detecting if a to_val function is provided for a type.
         *
         * @tparam T The type to check for
         */
        template <typename T>
        concept HasToVal = requires(T const& t, Nui::val& v) { to_val(v, t); };

        /**
         * @brief Concept detecting if a from_val function is provided for a type.
         *
         * @tparam T The type to check for
         */
        template <typename T>
        concept HasFromVal = requires(Nui::val const& v, T& t) { from_val(v, t); };
    }

    /**
     * @brief Converts a boost described class/struct to a Nui::val object.
     *
     * @tparam T The class type
     * @tparam Bases Boost describe bases
     * @tparam Members Boost describe members
     * @tparam Enable Used to disable unions for this function
     * @param obj The object to convert
     * @return Returns the val.
     */
    template <
        typename T,
        class Bases = boost::describe::describe_bases<T, boost::describe::mod_any_access>,
        class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>,
        class Enable = std::enable_if_t<!std::is_union_v<T>>>
    Nui::val convertToVal(T const& obj);

    /**
     * @brief Converts an optional<T> to a Nui::val
     *
     * @tparam T The wrapped type of the optional
     * @param option The optional value to convert
     * @return Nui::val The converted val
     */
    template <typename T>
    Nui::val convertToVal(std::optional<T> const& option);

    /**
     * @brief Converts fundamental types to Nui::val
     *
     * @tparam T The fundamental type
     * @param value The value to convert
     * @return Nui::val The converted val
     */
    template <typename T>
    requires Fundamental<T>
    Nui::val convertToVal(T const& value);

    /**
     * @brief Converts string to Nui::val.
     *
     * @param value The string to convert.
     * @return Nui::val The converted val.
     */
    Nui::val convertToVal(std::string const& value);

    /**
     * @brief Converts a filesystem path to Nui::val.
     *
     * @param value The filesystem path to convert.
     * @return Nui::val The converted val.
     */
    Nui::val convertToVal(std::filesystem::path const& value);

    /**
     * @brief Copy for val. Useful if a class itself contains a val.
     *
     * @param value The val to copy.
     * @return Nui::val The copied val.
     */
    Nui::val convertToVal(Nui::val value);

    /**
     * @brief Converts a C-style string to Nui::val.
     *
     * @param value The C-style string to convert.
     * @return Nui::val The converted val.
     */
    Nui::val convertToVal(char const* value);

    /**
     * @brief Converts a string_view to Nui::val.
     *
     * @param view The string_view to convert.
     * @return Nui::val The converted val.
     */
    Nui::val convertToVal(std::string_view value);

    /**
     * @brief Converts a vector to Nui::val.
     *
     * @tparam T The element type of the vector.
     * @param vector The vector to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    Nui::val convertToVal(std::vector<T> const& vector);

    /**
     * @brief Converts an Observed<T> to Nui::val.
     *
     * @tparam T The type of the observed value.
     * @param observed The observed value to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    Nui::val convertToVal(Observed<T> const& observed);

    /**
     * @brief Converts an unordered_map to Nui::val.
     *
     * @tparam T The value type of the map.
     * @param map The map to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    Nui::val convertToVal(std::unordered_map<std::string, T> const& map);

    /**
     * @brief Converts a map to Nui::val.
     *
     * @tparam T The value type of the map.
     * @param map The map to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    Nui::val convertToVal(std::map<std::string, T> const& map);

    /**
     * @brief Converts a unique pointer to Nui::val.
     *
     * @tparam T The type of the pointed-to object.
     * @param ptr The unique pointer to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    Nui::val convertToVal(std::unique_ptr<T> const& ptr);

    /**
     * @brief Converts a shared pointer to Nui::val.
     *
     * @tparam T The type of the pointed-to object.
     * @param ptr The shared pointer to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    Nui::val convertToVal(std::shared_ptr<T> const& ptr);

    /**
     * @brief Converts anything that a "to_val" function is provided for found by ADL.
     *
     * @tparam T The type of the object.
     * @param obj The object to convert.
     * @return Nui::val The converted val.
     */
    template <typename T>
    requires Detail::HasToVal<T>
    Nui::val convertToVal(T const& value);

#if __POINTER_WIDTH__ == 64
    /**
     * @brief Converts a long long to Nui::val. This is only possible when 64bit WASM is enabled.
     *
     * @param value The long long to convert.
     * @return Nui::val The converted val.
     */
    Nui::val convertToVal(long long value);

    /**
     * @brief Converts an unsigned long long to Nui::val. This is only possible when 64bit WASM is enabled.
     *
     * @param value The unsigned long long to convert.
     * @return Nui::val The converted val.
     */
    Nui::val convertToVal(unsigned long long value);
#else
    inline Nui::val convertToVal(long long)
    {
        throw std::runtime_error("Use -sMEMORY64 to convert long long to val");
    }
    inline Nui::val convertToVal(unsigned long long)
    {
        throw std::runtime_error("Use -sMEMORY64 to convert unsigned long long to val");
    }
#endif
    /**
     * @brief Converts a monostate to Nui::val (interpreted as undefined)
     *
     * @return Nui::val javascript undefined value.
     */
    inline Nui::val convertToVal(std::monostate)
    {
        return Nui::val::undefined();
    }

    /**
     * @brief Converts a variant to Nui::val by converting the currently held type.
     *
     * @tparam Ts
     * @param variant
     * @return Nui::val
     */
    template <typename... Ts>
    Nui::val convertToVal(std::variant<Ts...> const& variant);

    namespace Detail
    {
        template <typename T, class Members>
        requires(!std::is_union_v<T>)
        void convertFromValObjImpl(Nui::val const& val, T& obj);
    }

    /**
     * @brief Converts a Nui::val to a class/struct object.
     *
     * @tparam T The type of the object.
     * @tparam Bases The base classes of the object.
     * @tparam Members The members of the object.
     */
    template <typename T, class Bases, class Members>
    requires(!std::is_union_v<T>)
    void convertFromVal(Nui::val const& val, T& obj);

    /**
     * @brief Converts a Nui::val to a class/struct object.
     *
     * @tparam T The type of the object.
     * @tparam Members The boost describe members of the object.
     */
    template <typename T, class Members>
    requires(!std::is_union_v<T> && !boost::describe::has_describe_bases<T>::value)
    void convertFromVal(Nui::val const& val, T& obj);

    /**
     * @brief Converts a Nui::val to a fundamental type.
     *
     * @tparam T The fundamental type.
     * @param val The val to convert.
     * @param value The variable to store the converted value.
     */
    template <typename T>
    requires Fundamental<T>
    void convertFromVal(Nui::val const& val, T& value);

    /**
     * @brief Converts a Nui::val to a std::string.
     *
     * @param val The val to convert.
     * @param str The variable to store the converted value.
     */
    void convertFromVal(Nui::val const& val, std::string& str);

    /**
     * @brief Converts a Nui::val to a std::optional.
     *
     * @tparam T The wrapped type of the optional.
     * @param val The val to convert.
     * @param option The variable to store the converted value.
     */
    template <typename T>
    void convertFromVal(Nui::val const& val, std::optional<T>& option);

    /**
     * @brief Converts a Nui::val to a std::filesystem::path.
     *
     * @param val The val to convert.
     * @param value The variable to store the converted value.
     */
    void convertFromVal(Nui::val const& val, std::filesystem::path& value);

    /**
     * @brief Converts a Nui::val to a Nui::val (copy).
     *
     * @param val The val to convert.
     * @param value The variable to store the converted value.
     */
    void convertFromVal(Nui::val const& val, Nui::val& value);

    /**
     * @brief Converts a Nui::val to a std::vector.
     *
     * @tparam T The type of the vector elements.
     * @param val The val to convert.
     * @param vector The variable to store the converted value.
     */
    template <typename T>
    void convertFromVal(Nui::val const& val, std::vector<T>& vector);

    /**
     * @brief Converts a Nui::val to a std::vector of fundamentals.
     *
     * @tparam T The type of the vector elements.
     * @param val The val to convert.
     * @param vector The variable to store the converted value.
     */
    template <typename T>
    requires Fundamental<T>
    void convertFromVal(Nui::val const& val, std::vector<T>& vector);

    /**
     * @brief Converts a Nui::val to an Observed<T>.
     *
     * @tparam T The type of the observed value.
     * @param val The val to convert.
     * @param observed The variable to store the converted value.
     */
    template <typename T>
    void convertFromVal(Nui::val const& val, Observed<T>& observed);

    /**
     * @brief Converts a Nui::val to a std::unordered_map.
     *
     * @tparam T The type of the map values.
     * @param val The val to convert.
     * @param map The variable to store the converted value.
     */
    template <typename T>
    void convertFromVal(Nui::val const& val, std::unordered_map<std::string, T>& map);

    /**
     * @brief Converts a Nui::val to a type that has a "from_val" function provided found by ADL.
     *
     * @tparam T The type of the value.
     * @param val The val to convert.
     * @param value The variable to store the converted value.
     */
    template <typename T>
    requires Detail::HasFromVal<T>
    void convertFromVal(Nui::val const& val, T& value);

#if __POINTER_WIDTH__ == 64
    /**
     * @brief Converts a Nui::val to a long long. This is only possible when 64bit WASM is enabled.
     *
     * @param val The val to convert.
     * @param value The variable to store the converted value.
     */
    void convertFromVal(Nui::val const& val, long long& value);

    /**
     * @brief Converts a Nui::val to an unsigned long long. This is only possible when 64bit WASM is enabled.
     *
     * @param val The val to convert.
     * @param value The variable to store the converted value.
     */
    void convertFromVal(Nui::val const& val, unsigned long long& value);
#else
    inline void convertFromVal(Nui::val const&, long long)
    {
        throw std::invalid_argument("Use -sMEMORY64 to convert from val to long long");
    }

    inline void convertFromVal(Nui::val const& val, unsigned long long& value)
    {
        throw std::invalid_argument("Use -sMEMORY64 to convert from val to unsigned long long");
    }
#endif

    template <typename T, class Bases, class Members, class Enable>
    Nui::val convertToVal(T const& obj)
    {
        Nui::val val = Nui::val::object();

        boost::mp11::mp_for_each<Bases>([&](auto&& base) {
            using type = typename std::decay_t<decltype(base)>::type;
            convertToVal(val, static_cast<type const&>(obj));
        });

        boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
            val.set(memAccessor.name, convertToVal(obj.*memAccessor.pointer));
        });

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
    inline Nui::val convertToVal(std::string_view value)
    {
        return convertToVal(std::string{value});
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
    template <typename T>
    Nui::val convertToVal(std::map<std::string, T> const& map)
    {
        Nui::val result = Nui::val::object();
        for (auto const& [key, value] : map)
            result.set(key, convertToVal(value));
        return result;
    }
    template <typename T>
    Nui::val convertToVal(std::unique_ptr<T> const& ptr)
    {
        if (ptr)
            return convertToVal(*ptr);
        return Nui::val::null();
    }
    template <typename T>
    Nui::val convertToVal(std::shared_ptr<T> const& ptr)
    {
        if (ptr)
            return convertToVal(*ptr);
        return Nui::val::null();
    }
    template <typename... Ts>
    Nui::val convertToVal(std::variant<Ts...> const& variant)
    {
        return std::visit(
            [](auto&& value) {
                return convertToVal(value);
            },
            variant);
    }

    template <typename T>
    requires Detail::HasToVal<T>
    Nui::val convertToVal(T const& value)
    {
        Nui::val val;
        to_val(val, value);
        return val;
    }

#if __POINTER_WIDTH__ == 64
    inline Nui::val convertToVal(long long value)
    {
        return Nui::val{value};
    }
    inline Nui::val convertToVal(unsigned long long value)
    {
        return Nui::val{value};
    }
#endif

    template <typename T, class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>>
    requires(!std::is_union_v<T> && !boost::describe::has_describe_bases<T>::value)
    void convertFromVal(Nui::val const& val, T& obj)
    {
        Detail::convertFromValObjImpl<T, Members>(val, obj);
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
        Detail::convertFromValObjImpl<T, Members>(val, obj);
    }

    namespace Detail
    {
        template <typename T, class Members = boost::describe::describe_members<T, boost::describe::mod_any_access>>
        requires(!std::is_union_v<T>)
        void convertFromValObjImpl(Nui::val const& val, T& obj)
        {
            boost::mp11::mp_for_each<Members>([&](auto&& memAccessor) {
                if (val.hasOwnProperty(memAccessor.name))
                {
                    if constexpr (!Detail::IsOptional<std::decay_t<decltype(obj.*memAccessor.pointer)>>::value)
                    {
                        if (val[memAccessor.name].isNull() || val[memAccessor.name].isUndefined())
                        {
                            throw std::invalid_argument(
                                std::string{"Expected member "} + memAccessor.name + " to be non nullish");
                        }
                        convertFromVal(val[memAccessor.name], obj.*memAccessor.pointer);
                    }
                    else
                        convertFromVal(val[memAccessor.name], obj.*memAccessor.pointer);
                }
                else
                {
                    if constexpr (Detail::IsOptional<std::decay_t<decltype(obj.*memAccessor.pointer)>>::value)
                    {
                        // If the member is optional and not present, set to nullopt:
                        obj.*memAccessor.pointer = std::nullopt;
                    }
                    else
                    {
                        throw std::invalid_argument(std::string{"Missing required member "} + memAccessor.name);
                    }
                }
            });
        }
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

    template <typename T>
    requires Detail::HasFromVal<T>
    void convertFromVal(Nui::val const& val, T& value)
    {
        from_val(val, value);
    }

#if __POINTER_WIDTH__ == 64
    inline void convertFromVal(Nui::val const& val, long long& value)
    {
        value = val.as<long long>();
    }
    inline void convertFromVal(Nui::val const& val, unsigned long long& value)
    {
        value = val.as<unsigned long long>();
    }
#endif
}