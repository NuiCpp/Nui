#pragma once

#include <nui/feature_test.hpp>
#include <nui/frontend/elements/impl/materialize.hpp>
#include <nui/utility/iterator_accessor.hpp>
#include <nui/event_system/observed_value.hpp>
#include <nui/event_system/observed_value_combinator.hpp>

#include <utility>
#include <vector>
#include <functional>

#ifdef NUI_HAS_STD_RANGES
#    include <ranges>
#endif

namespace Nui
{
    namespace Dom
    {
        class Element;
    }

    template <typename Derived>
    class BasicObservedRange
    {
      public:
        BasicObservedRange() = default;
        virtual ~BasicObservedRange() = default;
        BasicObservedRange(BasicObservedRange const&) = default;
        BasicObservedRange(BasicObservedRange&&) = default;
        BasicObservedRange& operator=(BasicObservedRange const&) = default;
        BasicObservedRange& operator=(BasicObservedRange&&) = default;

        Derived& before(auto&& firstRenderer, auto&&... renderers) &
        {
            before_ = decltype(before_){
                std::forward<decltype(firstRenderer)>(firstRenderer), std::forward<decltype(renderers)>(renderers)...};
            return static_cast<Derived&>(*this);
        }
        Derived& after(auto&& firstRenderer, auto&&... renderers) &
        {
            after_ = decltype(after_){
                std::forward<decltype(firstRenderer)>(firstRenderer), std::forward<decltype(renderers)>(renderers)...};
            return static_cast<Derived&>(*this);
        }

        Derived&& before(auto&& firstRenderer, auto&&... renderers) &&
        {
            before_ = decltype(before_){
                std::forward<decltype(firstRenderer)>(firstRenderer), std::forward<decltype(renderers)>(renderers)...};
            return static_cast<Derived&&>(*this);
        }
        Derived&& after(auto&& firstRenderer, auto&&... renderers) &&
        {
            after_ = decltype(after_){
                std::forward<decltype(firstRenderer)>(firstRenderer), std::forward<decltype(renderers)>(renderers)...};
            return static_cast<Derived&&>(*this);
        }

        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> ejectBefore()
        {
            return std::move(before_);
        }
        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> ejectAfter()
        {
            return std::move(after_);
        }

        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> const& before() const
        {
            return before_;
        }

        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> const& after() const
        {
            return after_;
        }

      protected:
        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> before_{};
        std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>> after_{};
    };

    template <typename ObservedValue>
    class ObservedRange : public BasicObservedRange<ObservedRange<ObservedValue>>
    {
      public:
        using ObservedType = ObservedValue;
        using BasicObservedRange<ObservedRange<ObservedValue>>::before_;
        using BasicObservedRange<ObservedRange<ObservedValue>>::after_;

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
    class UnoptimizedRange : public BasicObservedRange<UnoptimizedRange<CopyableRangeLike, ObservedValues...>>
    {
      public:
        using BasicObservedRange<UnoptimizedRange<CopyableRangeLike, ObservedValues...>>::before;
        using BasicObservedRange<UnoptimizedRange<CopyableRangeLike, ObservedValues...>>::after;

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

    template <typename... MapTemplateArgs, typename... Observed>
    UnoptimizedRange<
        IteratorAccessor<std::unordered_map<MapTemplateArgs...>>,
        std::decay_t<Detail::ObservedAddReference_t<Observed>>...>
    range(std::unordered_map<MapTemplateArgs...>& container, Observed&&... observed)
    {
        return UnoptimizedRange<
            IteratorAccessor<std::unordered_map<MapTemplateArgs...>>,
            std::decay_t<Detail::ObservedAddReference_t<Observed>>...>{
            ObservedValueCombinator{std::forward<Detail::ObservedAddReference_t<Observed>>(observed)...},
            IteratorAccessor<std::unordered_map<MapTemplateArgs...>>{container},
        };
    }

    template <typename... MapTemplateArgs>
    UnoptimizedRange<
        IteratorAccessor<std::unordered_map<MapTemplateArgs...>>,
        std::decay_t<Detail::ObservedAddReference_t<Nui::Observed<std::unordered_map<MapTemplateArgs...>>>>>
    range(Nui::Observed<std::unordered_map<MapTemplateArgs...>>& container)
    {
        return range(container.value(), container);
    }

    template <typename... MapTemplateArgs, typename... Observed>
    UnoptimizedRange<
        IteratorAccessor<std::map<MapTemplateArgs...>>,
        std::decay_t<Detail::ObservedAddReference_t<Observed>>...>
    range(std::map<MapTemplateArgs...>& container, Observed&&... observed)
    {
        return UnoptimizedRange<
            IteratorAccessor<std::map<MapTemplateArgs...>>,
            std::decay_t<Detail::ObservedAddReference_t<Observed>>...>{
            ObservedValueCombinator{std::forward<Detail::ObservedAddReference_t<Observed>>(observed)...},
            IteratorAccessor<std::map<MapTemplateArgs...>>{container},
        };
    }

    template <typename... MapTemplateArgs>
    UnoptimizedRange<
        IteratorAccessor<std::map<MapTemplateArgs...>>,
        std::decay_t<Detail::ObservedAddReference_t<Nui::Observed<std::map<MapTemplateArgs...>>>>>
    range(Nui::Observed<std::map<MapTemplateArgs...>>& container)
    {
        return range(container.value(), container);
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
        std::decay_t<Detail::ObservedAddReference_t<Observed>>...> range(T const& container, Observed&&... observed)
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