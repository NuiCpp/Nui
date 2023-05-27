#pragma once

#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/utility/fixed_string.hpp>

#include <nui/frontend/val.hpp>

namespace Nui::Attributes
{
    namespace Detail
    {
        template <typename ElementT, typename T>
        EventContext::EventIdType defaultSetEvent(std::weak_ptr<ElementT> element, T const& obs, char const* name)
        {
            const auto eventId = globalEventContext.registerEvent(Event{
                [element, obs, name](auto eventId) {
                    if (auto shared = element.lock(); shared)
                    {
                        shared->setAttribute(name, obs.value());
                        return true;
                    }
                    obs.unattachEvent(eventId);
                    return false;
                },
                [element]() {
                    return !element.expired();
                }});
            obs.attachEvent(eventId);
            return eventId;
        }
    }

    class AttributeFactory
    {
      public:
        explicit constexpr AttributeFactory(char const* name)
            : name_{name}
        {}

        // Dont use this class like a value
        AttributeFactory(Attribute const&) = delete;
        AttributeFactory(Attribute&&) = delete;
        AttributeFactory& operator=(Attribute const&) = delete;
        AttributeFactory& operator=(Attribute&&) = delete;

        constexpr char const* name() const
        {
            return name_;
        };

        template <typename U>
        requires(!IsObserved<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U>)
        Attribute operator=(U val) const
        {
            return Attribute{[name = name(), val = std::move(val)](Dom::ChildlessElement& element) {
                element.setAttribute(name, val);
            }};
        }
        template <typename U>
        requires(IsObserved<std::decay_t<U>>)
        Attribute operator=(U& val) const
        {
            return Attribute{
                [name = name(), &val](Dom::ChildlessElement& element) {
                    element.setAttribute(name, val.value());
                },
                [name = name(), &val](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultSetEvent(std::move(element), Nui::Detail::CopiableObservedWrap{val}, name);
                },
                [&val](EventContext::EventIdType const& id) {
                    val.unattachEvent(id);
                }};
        }
        template <typename RendererType, typename... ObservedValues>
        Attribute
        operator=(ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...> const& combinator) const
        {
            return Attribute{
                [name = name(), combinator](Dom::ChildlessElement& element) {
                    element.setAttribute(name, combinator.value());
                },
                [name = name(), combinator](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultSetEvent(std::move(element), combinator, name);
                },
                [combinator](EventContext::EventIdType const& id) {
                    combinator.unattachEvent(id);
                }};
        }

        Attribute operator=(std::function<void(Nui::val)> func) const
        {
            return Attribute{[name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                element.setAttribute(name, [func](Nui::val val) {
                    func(val);
                    globalEventContext.executeActiveEventsImmediately();
                });
            }};
        }

        Attribute operator=(std::function<void()> func) const
        {
            return Attribute{[name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                element.setAttribute(name, [func](Nui::val) {
                    func();
                    globalEventContext.executeActiveEventsImmediately();
                });
            }};
        }

      private:
        char const* name_;
    };
}

#define MAKE_HTML_VALUE_ATTRIBUTE_RENAME(NAME, HTML_NAME) \
    namespace Nui::Attributes \
    { \
        static constexpr auto NAME = AttributeFactory{HTML_NAME}; \
    }

#define MAKE_HTML_VALUE_ATTRIBUTE(NAME) MAKE_HTML_VALUE_ATTRIBUTE_RENAME(NAME, #NAME)

#define MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, HTML_ACTUAL) \
    namespace Nui::Attributes \
    { \
        namespace Names \
        { \
            static constexpr auto Attr##NAME = fixToLower(HTML_ACTUAL); \
        } \
        static constexpr auto NAME = AttributeFactory{Names::Attr##NAME}; \
    }

#define MAKE_HTML_EVENT_ATTRIBUTE(NAME) MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, #NAME)