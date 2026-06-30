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

        // Unwrap through shared_ptr/weak_ptr<Observed> so the trait reads from the
        // underlying Observed rather than the smart pointer (which has no isRandomAccess).
        static constexpr bool isRandomAccess =
            Detail::ObservedAddMutableReference_raw<ObservedType>::isRandomAccess;

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

    /**
     * @brief Reactive range over a shared_ptr/weak_ptr<Observed<Container>>.
     *
     * The smart pointer is stored internally as a weak_ptr (matching how shared/weak
     * observed are treated elsewhere), locked per update, so the range reacts to
     * mutations and reassignment of the underlying Observed. The caller must keep the
     * shared_ptr alive for as long as the rendered range is mounted. Taken by value so
     * the weak_ptr conversion is well-formed for both shared_ptr and weak_ptr inputs.
     */
    template <typename ObservedValue>
    requires(IsSharedObserved<ObservedValue> || IsWeakObserved<ObservedValue>)
    ObservedRange<std::decay_t<ObservedValue>> range(ObservedValue observedValues)
    {
        return ObservedRange<std::decay_t<ObservedValue>>{std::move(observedValues)};
    }

    template <typename ContainerT, typename... Observed>
    requires(!IsObservedLike<std::remove_cvref_t<ContainerT>>)
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
    requires(!IsObservedLike<std::remove_cvref_t<ContainerT>>)
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
    requires(!IsObservedLike<std::remove_cvref_t<ContainerT>>)
    UnoptimizedRange<IteratorAccessor<ContainerT const>> range(ContainerT const& container)
    {
        return UnoptimizedRange<IteratorAccessor<ContainerT const>>{
            IteratorAccessor<ContainerT const>{container},
        };
    }

    /**
     * @brief Owning range overload for rvalue containers. Takes ownership so the
     *        iterated elements remain alive for the full lifetime of the render,
     *        even if the caller's local container goes out of scope immediately.
     */
    template <typename ContainerT>
    requires(!IsObservedLike<std::remove_cvref_t<ContainerT>>)
    UnoptimizedRange<OwningIteratorAccessor<std::remove_cvref_t<ContainerT>>>
    range(ContainerT&& container)
    {
        using Owned = std::remove_cvref_t<ContainerT>;
        return UnoptimizedRange<OwningIteratorAccessor<Owned>>{
            OwningIteratorAccessor<Owned>{std::forward<ContainerT>(container)},
        };
    }

#ifdef NUI_HAS_STD_RANGES
    template <typename T, typename... Observed>
    requires(!IsObservedLike<std::remove_cvref_t<T>>)
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

    template <typename ContainerT, typename Tags>
    constexpr auto ObservedContainer<ContainerT, Tags>::map(auto&& function) const
    {
        return std::pair<ObservedRange<Observed<ContainerT, Tags>>, std::decay_t<decltype(function)>>{
            ObservedRange<Observed<ContainerT, Tags>>{static_cast<Observed<ContainerT, Tags> const&>(*this)},
            std::forward<std::decay_t<decltype(function)>>(function),
        };
    }

    template <typename ContainerT, typename Tags>
    constexpr auto ObservedContainer<ContainerT, Tags>::map(auto&& function)
    {
        return std::pair<ObservedRange<Observed<ContainerT, Tags>>, std::decay_t<decltype(function)>>{
            ObservedRange<Observed<ContainerT, Tags>>{static_cast<Observed<ContainerT, Tags>&>(*this)},
            std::forward<std::decay_t<decltype(function)>>(function),
        };
    }
}