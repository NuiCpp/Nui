#pragma once

#include <nui/feature_test.hpp>
#include <nui/utility/iterator_accessor.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

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

        template <typename ObservedValueT>
        requires std::is_same_v<std::decay_t<ObservedValueT>, std::decay_t<ObservedValueT>>
        explicit constexpr ObservedRange(ObservedValueT&& observedValues)
            : observedValue_{std::forward<Detail::ObservedAddMutableReference_t<ObservedValue>>(observedValues)}
        {}

        Detail::ObservedAddReference_t<ObservedValue> underlying() const
        {
            return observedValue_;
        }
        Detail::ObservedAddMutableReference_t<ObservedValue> underlying()
        requires(!std::is_const_v<ObservedValue>)
        {
            return observedValue_;
        }

      private:
        Detail::ObservedAddMutableReference_t<ObservedValue> observedValue_;
    };

    template <typename CopyableRangeLike, typename... ObservedValues>
    class UnoptimizedRange
    {
      public:
        UnoptimizedRange(ObservedValueCombinator<ObservedValues...>&& observedValues, CopyableRangeLike&& rangeLike)
            : observedValues_{std::move(observedValues)}
            , rangeLike_{std::move(rangeLike)}
        {}
        explicit UnoptimizedRange(CopyableRangeLike&& rangeLike)
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
    UnoptimizedRange<IteratorAccessor<ContainerT const>, std::decay_t<Detail::ObservedAddReference_t<Observed>>...>
    range(ContainerT const& container, Observed&&... observed)
    {
        return UnoptimizedRange<
            IteratorAccessor<ContainerT const>,
            std::decay_t<Detail::ObservedAddReference_t<Observed>>...>{
            ObservedValueCombinator{std::forward<Detail::ObservedAddReference_t<Observed>>(observed)...},
            IteratorAccessor<ContainerT const>{container},
        };
    }

    template <typename ContainerT, typename... Observed>
    UnoptimizedRange<IteratorAccessor<ContainerT>, std::decay_t<Detail::ObservedAddReference_t<Observed>>...>
    range(ContainerT& container, Observed&&... observed)
    {
        return UnoptimizedRange<
            IteratorAccessor<ContainerT>,
            std::decay_t<Detail::ObservedAddReference_t<Observed>>...>{
            ObservedValueCombinator{std::forward<Detail::ObservedAddReference_t<Observed>>(observed)...},
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
    UnoptimizedRange<IteratorAccessor<ContainerT const>> range(ContainerT const& container)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT const>>{
            IteratorAccessor<ContainerT const>{container},
        };
    }

#ifdef NUI_HAS_STD_RANGES
    template <typename T, typename... Observed>
    UnoptimizedRange<
        std::ranges::subrange<std::ranges::iterator_t<T const>>,
        std::decay_t<Detail::ObservedAddReference_t<Observed>>...>
    range(T const& container, Observed&&... observed)
    {
        return UnoptimizedRange<
            std::ranges::subrange<std::ranges::iterator_t<T const>>,
            std::decay_t<Detail::ObservedAddReference_t<Observed>>...>{
            ObservedValueCombinator{std::forward<Detail::ObservedAddReference_t<Observed>>(observed)...},
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