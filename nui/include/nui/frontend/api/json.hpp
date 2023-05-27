#pragma once

#include <nui/frontend/val.hpp>

#include <string>

namespace Nui
{
    class JSON
    {
      public:
        static Nui::val parse(std::string const& jsonString)
        {
            return Nui::val::global("JSON").call<Nui::val>("parse", jsonString);
        }
        static Nui::val parse(Nui::val const& val)
        {
            return Nui::val::global("JSON").call<Nui::val>("parse", val);
        }
        static std::string stringify(Nui::val const& json)
        {
            return Nui::val::global("JSON").call<std::string>("stringify", json);
        }
        static std::string stringify(Nui::val const& json, Nui::val const& replacer, int indent)
        {
            return Nui::val::global("JSON").call<std::string>("stringify", json, replacer, indent);
        }
        static std::string stringify(Nui::val const& json, int indent)
        {
            return Nui::val::global("JSON").call<std::string>("stringify", json, Nui::val::null(), indent);
        }
    };
}