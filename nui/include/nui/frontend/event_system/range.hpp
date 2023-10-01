#pragma once

#include <nui/frontend/event_system/observed_value.hpp>

namespace Nui
{
    template <typename ObservedValue>
    class ObservedRange
    {
      public:
        using ObservedType = ObservedValue;

        static constexpr bool isRandomAccess = ObservedType::isRandomAccess;

        explicit constexpr ObservedRange(ObservedValue& observedValues)
            : observedValue_{observedValues}
        {}

        ObservedValue const& underlying() const
        {
            return observedValue_;
        }
        ObservedValue& underlying()
        requires(!std::is_const_v<ObservedValue>)
        {
            return observedValue_;
        }

      private:
        ObservedValue& observedValue_;
    };

    template <typename IteratorT>
    class StaticRange
    {
      public:
        static constexpr bool isRandomAccess = std::
            is_same_v<typename std::iterator_traits<IteratorT>::iterator_category, std::random_access_iterator_tag>;

        StaticRange(IteratorT begin, IteratorT end)
            : begin_{std::move(begin)}
            , end_{std::move(end)}
        {}

        IteratorT begin() const
        {
            return begin_;
        }
        IteratorT end() const
        {
            return end_;
        }

        StaticRange<IteratorT> underlying() const&
        {
            return *this;
        }
        StaticRange<IteratorT> underlying() &&
        {
            return std::move(*this);
        }

      private:
        IteratorT begin_;
        IteratorT end_;
    };

    template <typename ObservedValue>
    requires(IsObserved<ObservedValue>)
    ObservedRange<ObservedValue> range(ObservedValue& observedValues)
    {
        return ObservedRange<ObservedValue>{observedValues};
    }

    template <typename ObservedValue>
    requires(IsObserved<ObservedValue>)
    ObservedRange<const ObservedValue> range(ObservedValue const& observedValues)
    {
        return ObservedRange<const ObservedValue>{observedValues};
    }

    template <typename IteratorT>
    StaticRange<IteratorT> range(IteratorT begin, IteratorT end)
    {
        return StaticRange<IteratorT>{std::move(begin), std::move(end)};
    }

    template <typename ContainerT>
    StaticRange<typename ContainerT::const_iterator> range(ContainerT const& container)
    {
        return StaticRange<typename ContainerT::const_iterator>{std::begin(container), std::end(container)};
    }

    template <typename ContainerT>
    constexpr auto ObservedContainer<ContainerT>::map(auto&& function) const
    {
        return std::pair<ObservedRange<Observed<ContainerT>>, std::decay_t<decltype(function)>>{
            ObservedRange<Observed<ContainerT>>{static_cast<Observed<ContainerT> const&>(*this)},
            std::forward<std::decay_t<decltype(function)>>(function),
        };
    }
    template <typename ContainerT>
    constexpr auto ObservedContainer<ContainerT>::map(auto&& function)
    {
        return std::pair<ObservedRange<Observed<ContainerT>>, std::decay_t<decltype(function)>>{
            ObservedRange<Observed<ContainerT>>{static_cast<Observed<ContainerT>&>(*this)},
            std::forward<std::decay_t<decltype(function)>>(function),
        };
    }
}