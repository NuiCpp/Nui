#pragma once

#include <emscripten/val.h>

#include <string>

namespace Nui
{
    class JSON
    {
      public:
        static emscripten::val parse(std::string const& jsonString)
        {
            return emscripten::val::global("JSON").call<emscripten::val>("parse", jsonString);
        }
        static emscripten::val parse(emscripten::val const& val)
        {
            return emscripten::val::global("JSON").call<emscripten::val>("parse", val);
        }
        static std::string stringify(emscripten::val const& json)
        {
            return emscripten::val::global("JSON").call<std::string>("stringify", json);
        }
    };
}