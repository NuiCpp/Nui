#pragma once

#include <nui/frontend/attributes/impl/attribute.hpp>
#include <nui/frontend/dom/childless_element.hpp>
#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>
#include <nui/utility/fixed_string.hpp>

#include <nui/frontend/val.hpp>

#include <traits/functions.hpp>

#include <concepts>
#include <memory>
#include <functional>

namespace Nui::Attributes
{
    namespace Detail
    {
        /**
         * @brief The basic change event handler for attribute/properties on dom elements. It registers an event on the
         * observed value that will call the provided onLock function when the value changes. The onLock function should
         * return true if the event should continue to be listened to, or false if it should be detached. The event will
         * also be detached if the element is destroyed.
         *
         * @tparam ElementT The type of the element that the attribute is applied to.
         * @tparam T The type of the observed value. This has to outlive the element.
         * @param element A weak pointer to the element that the attribute is applied to.
         * @param obs The observed value that triggers the event when it changes.
         * @param onLock A function that is called when the observed value changes. It should return true if the event
         * should continue to be listened to, or false if it should be detached.
         * @return EventContext::EventIdType The ID of the registered event.
         */
        template <typename ElementT, typename T>
        EventContext::EventIdType changeEventHandler(std::weak_ptr<ElementT> element, T const& obs, auto onLock)
        {
            const auto eventId = globalEventContext.registerEvent(
                Event{
                    [element, obs, onLock = std::move(onLock)](auto eventId) {
                        if (auto shared = element.lock(); shared)
                            return onLock(*shared);

                        obs.detachEvent(eventId);
                        return false;
                    },
                    [element]() {
                        return !element.expired();
                    },
                });
            obs.attachEvent(eventId);
            return eventId;
        }

        /**
         * @brief Metafunction used to determine if a callable can be called by explicitly constructing the first
         * argument from a Nui::val. This enables wrappers for Nui::val to be used for event handlers without the need
         * for the user to explicitly construct it from a Nui::val while not making the construction implicit for the
         * wrapper, which could lead to unintended conversions.
         *
         * @tparam FunctionT
         */
        template <typename FunctionT>
        struct CallableByExplicitConstructionOfValImpl
        {
            static constexpr bool value = false;
        };

        template <typename FunctionT>
        requires Traits::CallableOfArity<FunctionT, 1>
        struct CallableByExplicitConstructionOfValImpl<FunctionT>
        {
            static constexpr bool value =
                std::is_constructible_v<typename Traits::FunctionTraits<FunctionT>::template Argument<0>, Nui::val>;
        };

        template <typename FunctionCallableByExplicitConstructionOfVal>
        concept CallableByExplicitConstructionOfVal =
            CallableByExplicitConstructionOfValImpl<std::decay_t<FunctionCallableByExplicitConstructionOfVal>>::value;

        /**
         * @brief Policy for setting properties on DOM elements.
         */
        struct SetPropertyPolicy
        {
            template <typename ValueT>
            static void set(Dom::ChildlessElement& element, char const* name, ValueT&& value) noexcept(
                noexcept(std::declval<Dom::ChildlessElement&>().setProperty(name, std::forward<ValueT>(value))))

            {
                element.setProperty(name, std::forward<ValueT>(value));
            }
        };

        /**
         * @brief Policy for setting attributes on DOM elements.
         */
        struct SetAttributePolicy
        {
            template <typename ValueT>
            static void set(Dom::ChildlessElement& element, char const* name, ValueT&& value) noexcept(
                noexcept(std::declval<Dom::ChildlessElement&>().setAttribute(name, std::forward<ValueT>(value))))
            {
                element.setAttribute(name, std::forward<ValueT>(value));
            }
        };

        /**
         * @brief Policy for setting the value of a text node on DOM elements.
         */
        struct TextNodeAttributePolicy
        {
            template <typename ValueT>
            static void set(Dom::ChildlessElement& element, char const*, ValueT&& value) noexcept
            {
                element.setNodeValue(std::forward<ValueT>(value));
            }
        };
    }

    /**
     * @brief This is the class that is used to create attributes/properties for DOM elements. It uses a policy to
     * determine how the attribute should be applied to the element. The factory can be assigned a stateful variable to
     * create an attribute/property. If the stateful variable is an observed value, the factory will automatically
     * register an event to update the attribute/property when the observed value changes. The factory can also be
     * assigned a plain value, in which case it will create an attribute/property with that value. The factory can also
     * be assigned a function, in which case it will create an event handler that calls the function when the event is
     * triggered.
     *
     * @tparam Policy How to apply the attribute to the element (e.g. as an attribute, as a property, or as a text node
     * value).
     */
    template <typename Policy>
    class ElementMemberFactory
    {
      public:
        explicit constexpr ElementMemberFactory(char const* name)
            : name_{name}
        {}

        constexpr ElementMemberFactory(ElementMemberFactory const& other) = default;
        constexpr ElementMemberFactory(ElementMemberFactory&& other) noexcept = default;
        ElementMemberFactory& operator=(ElementMemberFactory const&) = delete;
        ElementMemberFactory& operator=(ElementMemberFactory&&) = delete;
        ~ElementMemberFactory() = default;

        constexpr char const* name() const
        {
            return name_;
        };

        template <typename U>
        requires(
            !IsObservedLike<std::decay_t<U>> && !std::invocable<U, Nui::val> &&
            !Detail::CallableByExplicitConstructionOfVal<U> && !std::invocable<U>)
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(U val) const
        {
            return Attribute{
                [name = name(), val = std::move(val)](Dom::ChildlessElement& element) {
                    Policy::set(element, name, val);
                },
            };
        }

        template <typename U>
        requires(IsSharedObserved<std::decay_t<U>>)
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(U const& shared) const
        {
            return Attribute{
                [name = name(), weak = std::weak_ptr{shared}](Dom::ChildlessElement& element) {
                    if (auto shared = weak.lock(); shared)
                        Policy::set(element, name, shared->value());
                },
                [name = name(), weak = std::weak_ptr{shared}](std::weak_ptr<Dom::ChildlessElement> const& element) {
                    auto shared = weak.lock();
                    if (!shared)
                        return EventContext::invalidEventId;

                    const auto eventId = globalEventContext.registerEvent(
                        Event{
                            [name, element, obsWeak = std::weak_ptr{shared}](auto eventId) {
                                auto obsShared = obsWeak.lock();
                                if (!obsShared)
                                {
                                    return false;
                                }
                                if (auto shared = element.lock(); shared)
                                {
                                    Policy::set(*shared, name, obsShared->value());
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
        // NOLINTNEXTLINE
        Attribute operator=(U&& val) const
        {
            auto shared = val.lock();
            if (!shared)
                return Attribute{};

            return operator=(shared);
        }

        template <typename U>
        requires(IsObserved<std::decay_t<U>>)
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(U& val) const
        {
            return Attribute{
                [name = name(), &val](Dom::ChildlessElement& element) {
                    Policy::set(element, name, val.value());
                },
                [name = name(), &val](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::changeEventHandler(
                        element,
                        ::Nui::Detail::CopyableObservedWrap{val},
                        [name = name, obs = ::Nui::Detail::CopyableObservedWrap{val}](Dom::ChildlessElement& element) {
                            Policy::set(element, name, obs.value());
                            return true;
                        });
                },
                [&val](EventContext::EventIdType id) {
                    val.detachEvent(id);
                },
            };
        }

        template <typename U>
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(std::reference_wrapper<Observed<U>> refObs) const
        {
            return this->operator=(refObs.get());
        }

        template <typename RendererType, typename... ObservedValues>
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute
        operator=(ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...> const& combinator) const
        {
            return Attribute{
                [name = name(), combinator](Dom::ChildlessElement& element) {
                    Policy::set(element, name, combinator.value());
                },
                [name = name(), combinator](std::weak_ptr<Dom::ChildlessElement>&& element) {
                    return Detail::changeEventHandler(
                        element, combinator, [name = name, combinator](Dom::ChildlessElement& element) {
                            Policy::set(element, name, combinator.value());
                            return true;
                        });
                },
                [combinator](EventContext::EventIdType id) {
                    combinator.detachEvent(id);
                },
            };
        }

        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(std::function<void()> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    Policy::set(element, name, [func](Nui::val const&) mutable {
                        func();
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(std::function<void(Nui::val)> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    Policy::set(element, name, [func](Nui::val val) mutable {
                        func(std::move(val));
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        template <typename FunctionT>
        requires Detail::CallableByExplicitConstructionOfVal<FunctionT>
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(FunctionT func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    Policy::set(element, name, [func](Nui::val val) mutable {
                        func(
                            std::decay_t<typename Traits::FunctionTraits<FunctionT>::template Argument<0>>{
                                std::move(val)});
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

      private:
        char const* name_;
    };

    using PropertyFactory = ElementMemberFactory<Detail::SetPropertyPolicy>;
    using AttributeFactory = ElementMemberFactory<Detail::SetAttributePolicy>;

    /**
     * @brief The EventFactory is similar to the ElementMemberFactory but it can only be used for creating event
     * listeners on DOM elements.
     */
    class EventFactory
    {
      public:
        explicit constexpr EventFactory(char const* name)
            : name_{name}
        {}

        constexpr EventFactory(EventFactory const& other) = default;
        constexpr EventFactory(EventFactory&& other) = default;
        EventFactory& operator=(EventFactory const&) = delete;
        EventFactory& operator=(EventFactory&&) = delete;
        ~EventFactory() = default;

        constexpr char const* name() const
        {
            return name_;
        };

        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(std::function<void()> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.addEventListener(name, [func](Nui::val const&) mutable {
                        func();
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(std::function<void(Nui::val)> func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.addEventListener(name, [func](Nui::val val) mutable {
                        func(std::move(val));
                        globalEventContext.executeActiveEventsImmediately();
                    });
                },
            };
        }

        template <typename FunctionT>
        requires Detail::CallableByExplicitConstructionOfVal<FunctionT>
        // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
        Attribute operator=(FunctionT func) const
        {
            return Attribute{
                [name = name(), func = std::move(func)](Dom::ChildlessElement& element) {
                    element.addEventListener(name, [func](Nui::val val) mutable {
                        func(
                            std::decay_t<typename Traits::FunctionTraits<FunctionT>::template Argument<0>>{
                                std::move(val)});
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
        /**
         * @brief Creates an AttributeFactory with the given name. The returned factory can be used to create attributes
         * for DOM elements by assigning values to it.
         *
         * @param name The name of the attribute to create (e.g. "class", "id", "onclick", etc.).
         * @return constexpr AttributeFactory
         */
        constexpr AttributeFactory operator""_attr(char const* name, std::size_t)
        {
            return AttributeFactory{name};
        }

        /**
         * @brief Creates a PropertyFactory with the given name. The returned factory can be used to create properties
         * for DOM elements by assigning values to it.
         *
         * @param name The name of the property to create (e.g. "value", "checked", "disabled", etc.).
         * @return constexpr PropertyFactory
         */
        constexpr PropertyFactory operator""_prop(char const* name, std::size_t)
        {
            return PropertyFactory{name};
        }
        /**
         * @brief Creates an EventFactory with the given name. The returned factory can be used to create event
         * listeners for DOM elements by assigning functions to it.
         *
         * @param name The name of the event to create (e.g. "onclick", "onchange", "oninput", etc.).
         * @return constexpr EventFactory
         */
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
            // NOLINTNEXTLINE(misc-unconventional-assign-operator, cppcoreguidelines-c-copy-assignment-signature)
            Attribute operator=(Args&&... args) const
            {
                auto attr = factory.operator=(std::forward<Args>(args)...);
                attr.defer(true);
                return attr;
            };
        };
    }

    /**
     * @brief This operator can be used to defer the application of an attribute until the parent element is attached to
     * the DOM.
     *
     * @tparam T The type of the factory to defer (e.g. AttributeFactory, PropertyFactory, EventFactory).
     */
    template <typename T>
    requires(
        std::is_same_v<std::decay_t<T>, AttributeFactory> || std::is_same_v<std::decay_t<T>, PropertyFactory> ||
        std::is_same_v<std::decay_t<T>, EventFactory>)
    constexpr Detail::DeferWrap<T> operator!(T&& factory)
    {
        return Detail::DeferWrap<T>{.factory = std::forward<T>(factory)};
    }
}

// NOLINTBEGIN
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
// NOLINTEND

#define MAKE_HTML_EVENT_ATTRIBUTE(NAME) MAKE_HTML_EVENT_ATTRIBUTE_RENAME(NAME, #NAME)