#pragma once

#include <string>

namespace Nui::Reactive
{
    class Attribute
    {
      public:
        Attribute(std::string name, std::string value)
            : name_{std::move(name)}
            , value_{std::move(value)}
        {}

        const std::string& name() const
        {
            return name_;
        }

        const std::string& value() const
        {
            return value_;
        }

      private:
        std::string name_;
        std::string value_;
    };
}