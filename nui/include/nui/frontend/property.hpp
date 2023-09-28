#include <nui/frontend/event_system/observed_value.hpp>
#include <nui/frontend/event_system/observed_value_combinator.hpp>

#include <concepts>
#include <functional>

namespace Nui
{
    namespace Detail
    {
        template <typename T>
        struct Property
        {
            T prop;
        };

        template <typename T>
        struct Property<Observed<T>>
        {
            Observed<T> const* prop;
        };

        template <typename T>
        struct IsPropertyImpl
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct IsPropertyImpl<Property<T>>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        concept IsProperty = IsPropertyImpl<std::decay_t<T>>::value;
    }

    template <typename U>
    requires(IsObserved<std::decay_t<U>>)
    Detail::Property<std::decay_t<U>> property(U& val)
    {
        return Detail::Property<std::decay_t<U>>{.prop = &val};
    }

    template <typename U>
    requires(std::invocable<U, Nui::val>)
    Detail::Property<std::function<void(Nui::val)>> property(U val)
    {
        return Detail::Property<std::function<void(Nui::val)>>{.prop = std::move(val)};
    }

    template <typename U>
    requires(std::invocable<U>)
    Detail::Property<std::function<void()>> property(U val)
    {
        return Detail::Property<std::function<void()>>{.prop = std::move(val)};
    }

    template <typename U>
    requires(!IsObserved<std::decay_t<U>> && !std::invocable<U> && !std::invocable<U, Nui::val>)
    Detail::Property<std::decay_t<U>> property(U val)
    {
        return Detail::Property<std::decay_t<U>>{.prop = std::move(val)};
    }

    template <typename RendererType, typename... ObservedValues>
    ObservedValueCombinatorWithPropertyGenerator<RendererType, ObservedValues...>
    property(ObservedValueCombinatorWithGenerator<RendererType, ObservedValues...> combinator)
    {
        return ObservedValueCombinatorWithPropertyGenerator<RendererType, ObservedValues...>{std::move(combinator)};
    }
}