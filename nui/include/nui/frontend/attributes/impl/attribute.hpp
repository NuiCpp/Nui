#pragma once

#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/utility/fixed_string.hpp>

#include <emscripten/val.h>

#include <utility>
#include <string>
#include <type_traits>
#include <any>

namespace Nui
{
    template <typename T>
    class Attribute
    {
      public:
        constexpr static bool is_static_value = true;

        Attribute(char const* name)
            : name_{name}
            , value_{}
        {}
        Attribute(char const* name, T value)
            : name_{name}
            , value_{std::move(value)}
        {}

        Attribute(Attribute const&) = default;
        Attribute(Attribute&&) = default;
        Attribute& operator=(Attribute const&) = default;
        Attribute& operator=(Attribute&&) = default;

        char const* name() const
        {
            return name_;
        }

        T const& value() const
        {
            return value_;
        }

        void createEvent(auto&&, auto&&) const
        {}

      private:
        char const* name_;
        T value_;
    };

    template <typename T>
    class Attribute<Observed<T>>
    {
      public:
        constexpr static bool is_static_value = false;

        Attribute(char const* name, Observed<T>& value)
            : name_{name}
            , obs_{value}
        {}

        char const* name() const
        {
            return name_;
        }

        T const& value() const
        {
            return obs_.value();
        }

        Observed<T>& observed() const
        {
            return obs_;
        }

        template <typename ElementT>
        void createEvent(
            std::weak_ptr<ElementT> element,
            std::invocable<std::shared_ptr<ElementT> const&, T const&> auto event) const
        {
            const auto eventId = globalEventContext.registerEvent(Event{
                [element, event = std::move(event), &obs = this->obs_](auto) {
                    if (auto shared = element.lock(); shared)
                    {
                        event(shared, obs.value());
                        return true;
                    }
                    return false;
                },
                [element]() {
                    return !element.expired();
                }});
            obs_.attachEvent(eventId);
        }

      private:
        char const* name_;
        Observed<T>& obs_;
    };

    template <typename RendererType, typename... ObservedValueTypes>
    class Attribute<ObservedValueCombinatorWithGenerator<RendererType, ObservedValueTypes...>>
    {
      public:
        constexpr static bool is_static_value = false;

        Attribute(char const* name, ObservedValueCombinatorWithGenerator<RendererType, ObservedValueTypes...> value)
            : name_{name}
            , combinator_{std::move(value)}
        {}

        char const* name() const
        {
            return name_;
        }

        auto value() const
        {
            return combinator_.generate();
        }

        template <typename ElementT>
        void createEvent(
            std::weak_ptr<ElementT> element,
            std::invocable<std::shared_ptr<ElementT> const&, std::invoke_result_t<RendererType> const&> auto event)
            const
        {
            const auto eventId = globalEventContext.registerEvent(Event{
                [element, event = std::move(event), combinator_ = this->combinator_](auto eventId) {
                    if (auto shared = element.lock(); shared)
                    {
                        event(shared, combinator_.generate());
                        return true;
                    }
                    combinator_.unattachEvent(eventId);
                    return false;
                },
                [element]() {
                    return !element.expired();
                }});
            combinator_.attachEvent(eventId);
        }

      private:
        char const* name_;
        ObservedValueCombinatorWithGenerator<RendererType, ObservedValueTypes...> combinator_;
    };
}

#define MAKE_HTML_VALUE_ATTRIBUTE_RENAME(NAME, HTML_NAME) \
    namespace Nui::Attributes \
    { \
        struct NAME##Tag \
        { \
            constexpr static char const* name() \
            { \
                return HTML_NAME; \
            }; \
            template <typename U> \
            requires(!IsObserved<std::decay_t<U>>) \
            Attribute<std::decay_t<U>> operator=(U val) const \
            { \
                return Attribute<U>{name(), std::move(val)}; \
            } \
            template <typename U> \
            requires(IsObserved<std::decay_t<U>>) \
            Attribute<std::decay_t<U>> operator=(U& val) const \
            { \
                return Attribute<std::decay_t<U>>{name(), val}; \
            } \
            template <typename RendererType, typename... ObservedValues> \
            Attribute<ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...>> \
            operator=(ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...> const& combinator) const \
            { \
                return Attribute<ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...>>{ \
                    name(), combinator}; \
            } \
        } static constexpr NAME; \
    }

#define MAKE_HTML_VALUE_ATTRIBUTE(NAME) MAKE_HTML_VALUE_ATTRIBUTE_RENAME(NAME, #NAME)

#define MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Attributes \
    { \
        struct NAME##Tag \
        { \
            constexpr static auto nameValue = fixToLower(HTML_ACTUAL); \
\
            consteval static char const* name() \
            { \
                return nameValue; \
            }; \
            Attribute<std::function<void(emscripten::val)>> operator=(std::function<void(emscripten::val)> func) const \
            { \
                return Attribute<std::function<void(emscripten::val)>>{ \
                    name(), [func](emscripten::val val) { \
                        func(val); \
                        globalEventContext.executeActiveEventsImmediately(); \
                    }}; \
            } \
            Attribute<std::function<void(emscripten::val)>> operator=(std::function<void()> func) const \
            { \
                return Attribute<std::function<void(emscripten::val)>>{ \
                    name(), [func](emscripten::val) { \
                        func(); \
                        globalEventContext.executeActiveEventsImmediately(); \
                    }}; \
            } \
        } static constexpr NAME; \
    }

#define MAKE_HTML_EVENT_ATTRIBUTE(NAME) MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, #NAME)