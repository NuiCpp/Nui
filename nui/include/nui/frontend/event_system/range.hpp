#pragma once

#include <nui/feature_test.hpp>
#include <nui/utility/iterator_accessor.hpp>
#include <nui/frontend/event_system/observed_value.hpp>

#include <iterator>
#include <utility>

#ifdef NUI_HAS_STD_RANGES
#    include <ranges>
#endif

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

    template <typename CopyableRangeLike, typename... ObservedValues>
    class UnoptimizedRange
    {
      public:
        UnoptimizedRange(ObservedValueCombinator<ObservedValues...>&& observedValues, CopyableRangeLike&& rangeLike)
            : observedValues_{std::move(observedValues)}
            , rangeLike_{std::move(rangeLike)}
        {}
        UnoptimizedRange(CopyableRangeLike&& rangeLike)
        requires(sizeof...(ObservedValues) == 0)
            : observedValues_{}
            , rangeLike_{std::move(rangeLike)}
        {}

        auto begin() const
        {
            return rangeLike_.begin();
        }
        auto end() const
        {
            return rangeLike_.end();
        }

        ObservedValueCombinator<ObservedValues...> const& underlying() const
        {
            return observedValues_;
        }
        ObservedValueCombinator<ObservedValues...>& underlying()
        {
            return observedValues_;
        }

      private:
        ObservedValueCombinator<ObservedValues...> observedValues_;
        CopyableRangeLike rangeLike_;
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

    template <typename ContainerT, typename... Observed>
    UnoptimizedRange<IteratorAccessor<ContainerT const>, Observed...>
    range(ContainerT const& container, Observed const&... observed)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT const>, Observed...>{
            ObservedValueCombinator{observed...},
            IteratorAccessor<ContainerT const>{container},
        };
    }

    template <typename ContainerT, typename... Observed>
    UnoptimizedRange<IteratorAccessor<ContainerT>, Observed...>
    range(ContainerT& container, Observed const&... observed)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT>, Observed...>{
            ObservedValueCombinator{observed...},
            IteratorAccessor<ContainerT>{container},
        };
    }

    template <typename ContainerT, typename... Observed>
    UnoptimizedRange<IteratorAccessor<ContainerT const>, Observed...>
    range(ContainerT const& container, Observed&... observed)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT const>, Observed...>{
            ObservedValueCombinator{observed...},
            IteratorAccessor<ContainerT const>{container},
        };
    }

    template <typename ContainerT, typename... Observed>
    UnoptimizedRange<IteratorAccessor<ContainerT>, Observed...> range(ContainerT& container, Observed&... observed)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT>, Observed...>{
            ObservedValueCombinator{observed...},
            IteratorAccessor<ContainerT>{container},
        };
    }

    template <typename ContainerT>
    UnoptimizedRange<IteratorAccessor<ContainerT>> range(ContainerT& container)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT>>{
            IteratorAccessor<ContainerT>{container},
        };
    }

    template <typename ContainerT>
    UnoptimizedRange<IteratorAccessor<ContainerT>> range(ContainerT const& container)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT>>{
            IteratorAccessor<ContainerT>{container},
        };
    }

#ifdef NUI_HAS_STD_RANGES
    template <typename T, typename... Observed>
    UnoptimizedRange<std::ranges::subrange<std::ranges::iterator_t<T const>>, Observed...>
    range(T const& container, Observed const&... observed)
    {
        return UnoptimizedRange<std::ranges::subrange<std::ranges::iterator_t<T const>>, Observed...>{
            ObservedValueCombinator{observed...},
            std::ranges::subrange<std::ranges::iterator_t<T const>>{
                std::ranges::begin(container), std::ranges::end(container)},
        };
    }
#endif

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