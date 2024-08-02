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

        explicit constexpr PropertyFactory(PropertyFactory const& other)
            : name_{other.name_}
        {}
        explicit constexpr PropertyFactory(PropertyFactory&& other)
            : name_{other.name_}
        {}
        PropertyFactory& operator=(PropertyFactory const&) = delete;
        PropertyFactory& operator=(PropertyFactory&&) = delete;

        constexpr char const* name() const
        {
            return name_;
        };

        template <typename U>
        requires(
            !IsObservedLike<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U> &&
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
        requires(IsSharedObserved<std::decay_t<U>>)
        Attribute operator=(U const& shared) const
        {
            return Attribute{
                [name = name(), weak = std::weak_ptr{shared}](Dom::ChildlessElement& element) {
                    if (auto shared = weak.lock(); shared)
                        element.setProperty(name, shared->value());
                },
                [name = name(), weak = std::weak_ptr{shared}](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    auto shared = weak.lock();
                    if (!shared)
                        return EventContext::invalidEventId;

                    const auto eventId = globalEventContext.registerEvent(Event{
                        [name, element, obsWeak = std::weak_ptr{shared}](auto eventId) {
                            auto obsShared = obsWeak.lock();
                            if (!obsShared)
                            {
                                return false;
                            }
                            if (auto shared = element.lock(); shared)
                            {
                                shared->setProperty(name, obsShared->value());
                                return true;
                            }
                            obsShared->detachEvent(eventId);
                            return false;
                        },
                        [element, obsWeak = std::weak_ptr{shared}]() {
                            return !element.expired() && !obsWeak.expired();
                        },
                    });
                    shared->attachEvent(eventId);
                    return eventId;
                },
                [weak = std::weak_ptr{shared}](EventContext::EventIdType id) {
                    if (auto shared = weak.lock(); shared)
                        shared->detachEvent(id);
                },
            };
        }

        template <typename U>
        requires(IsWeakObserved<std::decay_t<U>>)
        Attribute operator=(U&& val) const
        {
            auto shared = val.lock();
            if (!shared)
                return Attribute{};

            return operator=(shared);
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
                [&val](EventContext::EventIdType id) {
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
                [combinator](EventContext::EventIdType id) {
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
                [combinator](EventContext::EventIdType id) {
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

        explicit constexpr AttributeFactory(AttributeFactory const& other)
            : name_{other.name_}
        {}
        explicit constexpr AttributeFactory(AttributeFactory&& other)
            : name_{other.name_}
        {}
        AttributeFactory& operator=(AttributeFactory const&) = delete;
        AttributeFactory& operator=(AttributeFactory&&) = delete;

        constexpr char const* name() const
        {
            return name_;
        };

        template <typename U>
        requires(
            !IsObservedLike<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U> &&
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
        requires(IsSharedObserved<std::decay_t<U>>)
        Attribute operator=(U const& shared) const
        {
            return Attribute{
                [name = name(), weak = std::weak_ptr{shared}](Dom::ChildlessElement& element) {
                    if (auto shared = weak.lock(); shared)
                        element.setAttribute(name, shared->value());
                },
                [name = name(), weak = std::weak_ptr{shared}](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    auto shared = weak.lock();
                    if (!shared)
                        return EventContext::invalidEventId;

                    const auto eventId = globalEventContext.registerEvent(Event{
                        [name, element, obsWeak = std::weak_ptr{shared}](auto eventId) {
                            auto obsShared = obsWeak.lock();
                            if (!obsShared)
                            {
                                return false;
                            }
                            if (auto shared = element.lock(); shared)
                            {
                                shared->setAttribute(name, obsShared->value());
                                return true;
                            }
                            obsShared->detachEvent(eventId);
                            return false;
                        },
                        [element, obsWeak = std::weak_ptr{shared}]() {
                            return !element.expired() && !obsWeak.expired();
                        },
                    });
                    shared->attachEvent(eventId);
                    return eventId;
                },
                [weak = std::weak_ptr{shared}](EventContext::EventIdType id) {
                    if (auto shared = weak.lock(); shared)
                        shared->detachEvent(id);
                },
            };
        }

        template <typename U>
        requires(IsWeakObserved<std::decay_t<U>>)
        Attribute operator=(U&& val) const
        {
            auto shared = val.lock();
            if (!shared)
                return Attribute{};

            return operator=(shared);
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
                [&val](EventContext::EventIdType id) {
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
                [p = prop.prop](EventContext::EventIdType id) {
                    p->detachEvent(id);
                },
            };
        }

        template <typename U>
        requires(IsWeakObserved<std::decay_t<U>>)
        Attribute operator=(Nui::Detail::Property<U> const& prop) const
        {
            auto shared = prop.prop.lock();
            if (!shared)
                return Attribute{};

            return Attribute{
                [name = name(), weak = std::weak_ptr{shared}](Dom::ChildlessElement& element) {
                    if (auto shared = weak.lock(); shared)
                        element.setProperty(name, shared->value());
                },
                [name = name(), weak = std::weak_ptr{shared}](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    auto shared = weak.lock();
                    if (!shared)
                        return EventContext::invalidEventId;

                    const auto eventId = globalEventContext.registerEvent(Event{
                        [name, element, obsWeak = std::weak_ptr{shared}](auto eventId) {
                            auto obsShared = obsWeak.lock();
                            if (!obsShared)
                            {
                                return false;
                            }
                            if (auto shared = element.lock(); shared)
                            {
                                shared->setProperty(name, obsShared->value());
                                return true;
                            }
                            obsShared->detachEvent(eventId);
                            return false;
                        },
                        [element, obsWeak = std::weak_ptr{shared}]() {
                            return !element.expired() && !obsWeak.expired();
                        },
                    });
                    shared->attachEvent(eventId);
                    return eventId;
                },
                [weak = std::weak_ptr{shared}](EventContext::EventIdType id) {
                    if (auto shared = weak.lock(); shared)
                        shared->detachEvent(id);
                },
            };
        }

        template <typename U>
        requires(!IsObservedLike<std::decay_t<U>> && !std::invocable<U, Nui::val> && !std::invocable<U>)
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
                [combinator](EventContext::EventIdType id) {
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
                [combinator](EventContext::EventIdType id) {
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

        explicit constexpr EventFactory(EventFactory const& other)
            : name_{other.name_}
        {}
        explicit constexpr EventFactory(EventFactory&& other)
            : name_{other.name_}
        {}
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

    namespace Detail
    {
        template <typename T>
        struct DeferWrap
        {
            T factory;

            template <typename... Args>
            Attribute operator=(Args&&... args) const
            {
                auto attr = factory.operator=(std::forward<Args>(args)...);
                attr.defer(true);
                return attr;
            };
        };
    }

    template <typename T>
    requires(
        std::is_same_v<T, AttributeFactory> || std::is_same_v<T, PropertyFactory> || std::is_same_v<T, EventFactory>)
    Detail::DeferWrap<T> operator!(T const& factory)
    {
        return Detail::DeferWrap<T>{.factory = T{std::move(factory)}};
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