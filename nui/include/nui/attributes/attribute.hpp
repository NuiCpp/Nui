#pragma once

#include <nui/observed.hpp>
#include <nui/utility/fixed_string.hpp>

#include <utility>
#include <string>
#include <any>

namespace Nui
{
    template <typename DiscreteAttribute, typename T, typename Enable = void>
    class Attribute
    {
      public:
        using discrete_attribute = DiscreteAttribute;
        constexpr static bool is_static_value = true;

        Attribute(T value)
            : value_{std::move(value)}
        {}

        Attribute(Attribute const&) = default;
        Attribute(Attribute&&) = default;
        Attribute& operator=(Attribute const&) = default;
        Attribute& operator=(Attribute&&) = default;

        constexpr static char const* name()
        {
            return discrete_attribute::name();
        }

        T const& value() const
        {
            return value_;
        }

        void emplaceSideEffect(auto&&) const
        {}

      private:
        T value_;
    };

    template <typename DiscreteAttribute, typename T>
    class Attribute<DiscreteAttribute, Observed<T>, void>
    {
      public:
        using discrete_attribute = DiscreteAttribute;
        constexpr static bool is_static_value = false;

        Attribute(Observed<T>& value)
            : obs_{value}
        {}

        constexpr static char const* name()
        {
            return discrete_attribute::name();
        }

        T const& value() const
        {
            return obs_.value();
        }

        Observed<T>& observed() const
        {
            return obs_;
        }

        void emplaceSideEffect(std::invocable<T const&> auto&& sideEffect) const
        {
            obs_.emplaceSideEffect(std::forward<std::decay_t<decltype(sideEffect)>>(sideEffect));
        }

      private:
        Observed<T>& obs_;
    };
}

#define MAKE_HTML_STRING_ATTRIBUTE(NAME) \
    namespace Nui::Attributes \
    { \
        struct NAME##_ \
        { \
            constexpr static char const* name() \
            { \
                return #NAME; \
            }; \
            template <typename U> \
            std::enable_if_t<!Detail::IsObserved_v<std::decay_t<U>>, Attribute<NAME##_, U>> operator=(U&& val) \
            { \
                return Attribute<NAME##_, U>{std::forward<U>(val)}; \
            } \
            template <typename U> \
            std::enable_if_t<Detail::IsObserved_v<std::decay_t<U>>, Attribute<NAME##_, std::decay_t<U>>> \
            operator=(U& val) \
            { \
                return Attribute<NAME##_, std::decay_t<U>>{val}; \
            } \
        } NAME; \
    }

#define MAKE_HTML_EVENT_ATTRIBUTE(NAME) \
    namespace Nui::Attributes \
    { \
        struct NAME##_ \
        { \
\
            constexpr static auto nameValue = fixToLower(#NAME); \
\
            consteval static char const* name() \
            { \
                return nameValue; \
            }; \
            Attribute<NAME##_, std::function<void(emscripten::val)>> \
            operator=(std::function<void(emscripten::val)>&& func) \
            { \
                return Attribute<NAME##_, std::function<void(emscripten::val)>>{std::move(func)}; \
            } \
        } NAME; \
    }