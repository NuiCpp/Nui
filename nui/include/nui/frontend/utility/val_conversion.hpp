#pragma once

#include <emscripten/val.h>

#include <optional>
#include <vector>
#include <filesystem>

namespace Nui
{
    template <typename T>
    emscripten::val to_val(std::optional<T> const& option)
    {
        return option ? emscripten::val(*option) : emscripten::val::undefined();
    }
    template <typename T>
    emscripten::val to_val(T const& value)
    {
        return emscripten::val{value};
    }
    emscripten::val to_val(std::filesystem::path const& value)
    {
        return emscripten::val{value.string()};
    }
    emscripten::val to_val(emscripten::val value)
    {
        return value;
    }
    emscripten::val to_val(char const* value)
    {
        return emscripten::val{std::string{value}};
    }
    template <typename T>
    emscripten::val to_val(std::vector<T> const& vector)
    {
        emscripten::val result = emscripten::val::array();
        for (auto const& element : vector)
            result.call<void>("push", to_val(element));
        return result;
    }
}