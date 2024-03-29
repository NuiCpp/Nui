#pragma once

#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/utility/fixed_string.hpp>
#include <nui/frontend/property.hpp>

#include <nui/frontend/val.hpp>

namespace Nui::Attributes
{
    namespace Detail
    {
        template <typename ElementT, typename T>
        EventContext::EventIdType changeEventHandler(
            std::weak_ptr<ElementT> element,
            T const& obs,
            std::function<bool(std::shared_ptr<ElementT> const&)> onLock)
        {
            const auto eventId = globalEventContext.registerEvent(Event{
                [element, obs, onLock = std::move(onLock)](auto eventId) {
                    if (auto shared = element.lock(); shared)
                    {
                        return onLock(shared);
                    }
                    obs.detachEvent(eventId);
                    return false;
                },
                [element]() {
                    return !element.expired();
                }});
            obs.attachEvent(eventId);
            return eventId;
        }

        template <typename ElementT, typename T>
        EventContext::EventIdType defaultAttributeEvent(std::weak_ptr<ElementT> element, T const& obs, char const* name)
        {
            return changeEventHandler(
                element,
                obs,
                std::function<bool(std::shared_ptr<ElementT> const&)>{
                    [name, obs](std::shared_ptr<ElementT> const& shared) {
                        shared->setAttribute(name, obs.value());
                        return true;
                    }});
        }

        template <typename ElementT, typename T>
        EventContext::EventIdType defaultPropertyEvent(std::weak_ptr<ElementT> element, T const& obs, char const* name)
        {
            return changeEventHandler(
                element,
                obs,
                std::function<bool(std::shared_ptr<ElementT> const&)>{
                    [name, obs](std::shared_ptr<ElementT> const& shared) {
                        shared->setProperty(name, obs.value());
                        return true;
                    }});
        }
    }

    class PropertyFactory
    {
      public:
        explicit constexpr PropertyFactory(char const* name)
            : name_{name}
        {}

        // Dont use this class like a value
        PropertyFactory(PropertyFactory const&) = delete;
        PropertyFactory(PropertyFactory&&) = delete;
        PropertyFactory& operator=(PropertyFactory const&) = delete;
        PropertyFactory& operator=(PropertyFactory&&) = delete;

        constexpr char const* name() const
        {
            return name_;
        };

        template <typename U>
        requires(
            !IsObserved<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U> &&
            !Nui::Detail::IsProperty<std::decay_t<U>>)
        Attribute operator=(U val) const
        {
            return Attribute{
                [name = name(), val = std::move(val)](Dom::ChildlessElement& element) {
                    element.setProperty(name, val);
                },
            };
        }

        template <typename U>
        requires(IsObserved<std::decay_t<U>>)
        Attribute operator=(U& val) const
        {
            return Attribute{
                [name = name(), &val](Dom::ChildlessElement& element) {
                    element.setProperty(name, val.value());
                },
                [name = name(), &val](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultPropertyEvent(
                        std::move(element), Nui::Detail::CopyableObservedWrap{val}, name);
                },
                [&val](EventContext::EventIdType const& id) {
                    val.detachEvent(id);
                },
            };
        }

        template <typename RendererType, typename... ObservedValues>
        Attribute
        operator=(ObservedValueCombinatorWithPropertyGenerator<RendererType, ObservedValues...> const& combinator) const
        {
            return Attribute{
                [name = name(), combinator](Dom::ChildlessElement& element) {
                    element.setProperty(name, combinator.value());
                },
                [name = name(), combinator](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultPropertyEvent(std::move(element), combinator, name);
                },
                [combinator](EventContext::EventIdType const& id) {
                    combinator.detachEvent(id);
                },
            };
        }

        template <typename RendererType, typename... ObservedValues>
        Attribute
        operator=(ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...> const& combinator) const
        {
            return Attribute{
                [name = name(), combinator](Dom::ChildlessElement& element) {
                    element.setProperty(name, combinator.value());
                },
                [name = name(), combinator](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultPropertyEvent(std::move(element), combinator, name);
                },
                [combinator](EventContext::EventIdType const& id) {
                    combinator.detachEvent(id);
                },
            };
        }

        Attribute operator=(std::function<void(Nui::val)> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.setProperty(name, [func](Nui::val val) {
                        func(val);
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        Attribute operator=(std::function<void()> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.setProperty(name, [func](Nui::val) {
                        func();
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

      private:
        char const* name_;
    };

    class AttributeFactory
    {
      public:
        explicit constexpr AttributeFactory(char const* name)
            : name_{name}
        {}

        // Dont use this class like a value
        AttributeFactory(AttributeFactory const&) = delete;
        AttributeFactory(AttributeFactory&&) = delete;
        AttributeFactory& operator=(AttributeFactory const&) = delete;
        AttributeFactory& operator=(AttributeFactory&&) = delete;

        constexpr char const* name() const
        {
            return name_;
        };

        template <typename U>
        requires(
            !IsObserved<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U> &&
            !Nui::Detail::IsProperty<std::decay_t<U>>)
        Attribute operator=(U val) const
        {
            return Attribute{
                [name = name(), val = std::move(val)](Dom::ChildlessElement& element) {
                    element.setAttribute(name, val);
                },
            };
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
                    return Detail::defaultAttributeEvent(
                        std::move(element), Nui::Detail::CopyableObservedWrap{val}, name);
                },
                [&val](EventContext::EventIdType const& id) {
                    val.detachEvent(id);
                },
            };
        }

        template <typename U>
        requires(IsObserved<std::decay_t<U>>)
        Attribute operator=(Nui::Detail::Property<U> const& prop) const
        {
            return Attribute{
                [name = name(), p = prop.prop](Dom::ChildlessElement& element) {
                    element.setProperty(name, p->value());
                },
                [name = name(), p = prop.prop](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultPropertyEvent(
                        std::move(element), Nui::Detail::CopyableObservedWrap{*p}, name);
                },
                [p = prop.prop](EventContext::EventIdType const& id) {
                    p->detachEvent(id);
                },
            };
        }

        template <typename U>
        requires(!IsObserved<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U>)
        Attribute operator=(Nui::Detail::Property<U> const& prop) const
        {
            return Attribute{[name = name(), p = std::move(prop.prop)](Dom::ChildlessElement& element) {
                element.setProperty(name, p);
            }};
        }

        template <typename RendererType, typename... ObservedValues>
        Attribute
        operator=(ObservedValueCombinatorWithPropertyGenerator<RendererType, ObservedValues...> const& combinator) const
        {
            return Attribute{
                [name = name(), combinator](Dom::ChildlessElement& element) {
                    element.setProperty(name, combinator.value());
                },
                [name = name(), combinator](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::defaultPropertyEvent(std::move(element), combinator, name);
                },
                [combinator](EventContext::EventIdType const& id) {
                    combinator.detachEvent(id);
                },
            };
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
                    return Detail::defaultAttributeEvent(std::move(element), combinator, name);
                },
                [combinator](EventContext::EventIdType const& id) {
                    combinator.detachEvent(id);
                },
            };
        }

        Attribute operator=(std::function<void(Nui::val)> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.setAttribute(name, [func](Nui::val val) {
                        func(val);
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        Attribute operator=(std::function<void()> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.setAttribute(name, [func](Nui::val) {
                        func();
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        Attribute operator=(Nui::Detail::Property<std::function<void(Nui::val)>> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func.prop)](Dom::ChildlessElement& element) {
                    element.setProperty(name, [func](Nui::val val) {
                        func(val);
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        Attribute operator=(Nui::Detail::Property<std::function<void()>> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func.prop)](Dom::ChildlessElement& element) {
                    element.setProperty(name, [func](Nui::val) {
                        func();
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

      private:
        char const* name_;
    };

    class EventFactory
    {
      public:
        explicit constexpr EventFactory(char const* name)
            : name_{name}
        {}

        // Dont use this class like a value
        EventFactory(EventFactory const&) = delete;
        EventFactory(EventFactory&&) = delete;
        EventFactory& operator=(EventFactory const&) = delete;
        EventFactory& operator=(EventFactory&&) = delete;

        constexpr char const* name() const
        {
            return name_;
        };

        Attribute operator=(std::function<void()> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.addEventListener(name, [func](Nui::val) {
                        func();
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        Attribute operator=(std::function<void(Nui::val)> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.addEventListener(name, [func](Nui::val val) {
                        func(std::move(val));
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

      private:
        char const* name_;
    };

    inline namespace Literals
    {
        constexpr AttributeFactory operator""_attr(char const* name, std::size_t)
        {
            return AttributeFactory{name};
        }
        constexpr PropertyFactory operator""_prop(char const* name, std::size_t)
        {
            return PropertyFactory{name};
        }
        constexpr EventFactory operator""_event(char const* name, std::size_t)
        {
            return EventFactory{name};
        }
    }
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