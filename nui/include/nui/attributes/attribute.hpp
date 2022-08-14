#pragma once

#include <string>
#include <any>

namespace Nui
{
    template <typename DiscreteAttribute, typename T>
    class Attribute
    {
      public:
        using discrete_attribute = DiscreteAttribute;

        Attribute(T value)
            : value_{std::move(value)}
        {}

        constexpr static char const* name()
        {
            return discrete_attribute::name;
        }

        Attribute(Attribute const&) = default;
        Attribute(Attribute&&) = default;

        T const& value() const
        {
            return value_;
        }

      private:
        T value_;
    };

    template <typename DiscreteAttribute, typename T>
    Attribute(DiscreteAttribute attr, T value) -> Attribute<DiscreteAttribute, T>;
}

#define MAKE_HTML_STRING_ATTRIBUTE(NAME) \
    namespace Nui \
    { \
        struct NAME##_ \
        { \
            constexpr static char const* name = #NAME; \
            template <typename U> \
            Attribute<NAME##_, U> operator=(U&& val) \
            { \
                return {std::forward<U>(val)}; \
            } \
        } NAME; \
    }