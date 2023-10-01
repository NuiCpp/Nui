namespace Nui::Detail
{
    enum RangeStillValid : bool
    {
        InvalidateRange = false,
        KeepRange = true,
    };

    template <typename RangeT, typename GeneratorT>
    class BasicObservedRenderer
    {
      public:
        using ObservedType = typename RangeT::ObservedType;

        template <typename Generator = GeneratorT>
        BasicObservedRenderer(ObservedType const& observed, Generator&& elementRenderer)
            : valueRange_{observed}
            , elementRenderer_{std::forward<GeneratorT>(elementRenderer)}
        {}
        virtual ~BasicObservedRenderer() = default;
        BasicObservedRenderer(BasicObservedRenderer const&) = delete;
        BasicObservedRenderer(BasicObservedRenderer&&) = delete;
        BasicObservedRenderer& operator=(BasicObservedRenderer const&) = delete;
        BasicObservedRenderer& operator=(BasicObservedRenderer&&) = delete;

      protected:
        bool fullRangeUpdate(auto& parent) const
        {
            if (valueRange_.rangeContext().isFullRangeUpdate())
            {
                parent->clearChildren();
                long long counter = 0;
                for (auto& element : valueRange_.value())
                    elementRenderer_(counter++, element)(*parent, Renderer{.type = RendererType::Append});
                return true;
            }
            return false;
        }

        virtual bool updateChildren() const = 0;

        void operator()(auto& materialized)
        {
            weakMaterialized_ = materialized;
            valueRange_.attachEvent(Nui::globalEventContext.registerEvent(Event{
                [self = this->shared_from_this()](int) -> bool {
                    return self->updateChildren();
                },
                [this /* fine because other function holds this */]() {
                    return !weakMaterialized_.expired();
                },
            }));
            updateChildren();
        }

      protected:
        ObservedType const& valueRange_;
        GeneratorT elementRenderer_;
        std::weak_ptr<Nui::Dom::Element> weakMaterialized_;
    };

    template <typename RangeT, typename GeneratorT>
    class RangeRenderer<RangeT, GeneratorT, true>
        : public BasicObservedRenderer<RangeT, GeneratorT>
        , public std::enable_shared_from_this<RangeRenderer<RangeT, GeneratorT, true>>
    {
      public:
        using BasicObservedRenderer<RangeT, GeneratorT>::BasicObservedRenderer;
        using BasicObservedRenderer<RangeT, GeneratorT>::weakMaterialized_;
        using BasicObservedRenderer<RangeT, GeneratorT>::valueRange_;
        using BasicObservedRenderer<RangeT, GeneratorT>::elementRenderer_;

        bool fullRangeUpdate(auto& parent) const
        {
            if (valueRange_.rangeContext().isFullRangeUpdate())
            {
                parent->clearChildren();
                long long counter = 0;
                for (auto& element : valueRange_.value())
                    elementRenderer_(counter++, element)(*parent, Renderer{.type = RendererType::Append});
                return true;
            }
            return false;
        }

        void insertions(auto& parent) const
        {
            if (const auto insertInterval = valueRange_.rangeContext().insertInterval(); insertInterval)
            {
                for (auto i = insertInterval->low(); i <= insertInterval->high(); ++i)
                {
                    elementRenderer_(i, valueRange_.value()[static_cast<std::size_t>(i)])(
                        *parent, Renderer{.type = RendererType::Insert, .metadata = static_cast<std::size_t>(i)});
                }
            }
        }

        void updates(auto& parent) const
        {
            for (auto const& range : valueRange_.rangeContext())
            {
                switch (range.type())
                {
                    case RangeStateType::Keep:
                    {
                        continue;
                    }
                    case RangeStateType::Modify:
                    {
                        for (auto i = range.low(), high = range.high(); i <= high; ++i)
                        {
                            elementRenderer_(i, valueRange_.value()[static_cast<std::size_t>(i)])(
                                *(*parent)[static_cast<std::size_t>(i)], Renderer{.type = RendererType::Replace});
                        }
                        break;
                    }
                    default:
                        break;
                }
            }
        }

        bool updateChildren() const override
        {
            auto parent = weakMaterialized_.lock();
            if (!parent)
                return InvalidateRange;

            // Regenerate all elements if necessary:
            if (fullRangeUpdate(parent))
                return KeepRange;

            insertions(parent);
            updates(parent);

            return KeepRange;
        }

        void operator()(auto& materialized)
        {
            weakMaterialized_ = materialized;
            valueRange_.attachEvent(Nui::globalEventContext.registerEvent(Event{
                [self = this->shared_from_this()](int) -> bool {
                    return self->updateChildren();
                },
                [this /* fine because other function holds this */]() {
                    return !weakMaterialized_.expired();
                },
            }));
            updateChildren();
        }
    };

    template <typename RangeT, typename GeneratorT>
    class RangeRenderer<RangeT, GeneratorT, false>
        : public BasicObservedRenderer<RangeT, GeneratorT>
        , public std::enable_shared_from_this<RangeRenderer<RangeT, GeneratorT, false>>
    {
      public:
        using BasicObservedRenderer<RangeT, GeneratorT>::BasicObservedRenderer;
        using BasicObservedRenderer<RangeT, GeneratorT>::weakMaterialized_;
        using BasicObservedRenderer<RangeT, GeneratorT>::valueRange_;
        using BasicObservedRenderer<RangeT, GeneratorT>::elementRenderer_;

        bool updateChildren() const override
        {
            auto parent = weakMaterialized_.lock();
            if (!parent)
                return InvalidateRange;

            fullRangeUpdate(parent);
            return KeepRange;
        }
    };

    template <typename RangeLike, typename GeneratorT, typename... ObservedT>
    class UnoptimizedRangeRenderer
        : public std::enable_shared_from_this<UnoptimizedRangeRenderer<RangeLike, GeneratorT, ObservedT...>>
    {
      public:
        template <typename Generator = GeneratorT>
        UnoptimizedRangeRenderer(
            UnoptimizedRange<RangeLike, ObservedT...>&& unoptimizedRange,
            Generator&& elementRenderer)
            : unoptimizedRange_{std::move(unoptimizedRange)}
            , elementRenderer_{std::forward<GeneratorT>(elementRenderer)}
        {}

        bool updateChildren() const
        {
            auto materialized = weakMaterialized_.lock();
            if (!materialized)
                return InvalidateRange;

            materialized->clearChildren();

            long long counter = 0;
            for (auto& element : unoptimizedRange_)
            {
                elementRenderer_(counter++, element)(*materialized, Renderer{.type = RendererType::Append});
            }

            return KeepRange;
        }

        void operator()(auto& materialized)
        {
            weakMaterialized_ = materialized;
            unoptimizedRange_.underlying().attachEvent(Nui::globalEventContext.registerEvent(Event{
                [self = this->shared_from_this()](int) -> bool {
                    return self->updateChildren();
                },
                [this]() {
                    return !weakMaterialized_.expired();
                },
            }));
            updateChildren();
        }

      protected:
        UnoptimizedRange<RangeLike, ObservedT...> unoptimizedRange_;
        GeneratorT elementRenderer_;
        std::weak_ptr<Nui::Dom::Element> weakMaterialized_;
    };
}