#pragma once

#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/event_system/observed_value_combinator.hpp>
#include <nui/utility/fixed_string.hpp>

#include <utility>
#include <string>
#include <type_traits>
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

        void createEvent(auto&&, auto&&) const
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
        Observed<T>& obs_;
    };

    template <typename DiscreteAttribute, typename GeneratorType, typename... ObservedValueTypes>
    class Attribute<DiscreteAttribute, ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValueTypes...>, void>
    {
      public:
        using discrete_attribute = DiscreteAttribute;
        constexpr static bool is_static_value = false;

        Attribute(ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValueTypes...> value)
            : combinator_{std::move(value)}
        {}

        constexpr static char const* name()
        {
            return discrete_attribute::name();
        }

        auto value() const
        {
            return combinator_();
        }

        template <typename ElementT>
        void createEvent(
            std::weak_ptr<ElementT> element,
            std::invocable<std::shared_ptr<ElementT> const&, std::invoke_result_t<GeneratorType> const&> auto event)
            const
        {
            const auto eventId = globalEventContext.registerEvent(Event{
                [element, event = std::move(event), combinator_ = this->combinator_](auto eventId) {
                    if (auto shared = element.lock(); shared)
                    {
                        event(shared, combinator_());
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
        ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValueTypes...> combinator_;
    };
}

#define MAKE_HTML_VALUE_ATTRIBUTE_RENAME(NAME, HTML_NAME) \
    namespace Nui::Attributes \
    { \
        struct NAME##Tag \
        { \
            constexpr static char const* name() \
            { \
                return #HTML_NAME; \
            }; \
            template <typename U> \
            std::enable_if_t<!::Nui::Detail::IsObserved_v<std::decay_t<U>>, Attribute<NAME##Tag, U>> \
            operator=(U val) const \
            { \
                return Attribute<NAME##Tag, U>{std::move(val)}; \
            } \
            template <typename U> \
            std::enable_if_t<::Nui::Detail::IsObserved_v<std::decay_t<U>>, Attribute<NAME##Tag, std::decay_t<U>>> \
            operator=(U& val) const \
            { \
                return Attribute<NAME##Tag, std::decay_t<U>>{val}; \
            } \
            template <typename GeneratorType, typename... ObservedValues> \
            Attribute<NAME##Tag, ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...>> \
            operator=(ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...> const& combinator) const \
            { \
                return Attribute<NAME##Tag, ObservedValueCombinatorWithGenerator<GeneratorType, ObservedValues...>>{ \
                    combinator}; \
            } \
        } static constexpr NAME; \
    }

#define MAKE_HTML_VALUE_ATTRIBUTE(NAME) MAKE_HTML_VALUE_ATTRIBUTE_RENAME(NAME, NAME)

#define MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Attributes \
    { \
        struct NAME##Tag \
        { \
            constexpr static auto nameValue = fixToLower(#HTML_ACTUAL); \
\
            consteval static char const* name() \
            { \
                return nameValue; \
            }; \
            Attribute<NAME##Tag, std::function<void(emscripten::val)>> \
            operator=(std::function<void(emscripten::val)> func) const \
            { \
                return Attribute<NAME##Tag, std::function<void(emscripten::val)>>{[func](emscripten::val val) { \
                    func(val); \
                    globalEventContext.executeActiveEventsImmediately(); \
                }}; \
            } \
        } static constexpr NAME; \
    }

#define MAKE_HTML_EVENT_ATTRIBUTE(NAME) MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, NAME)