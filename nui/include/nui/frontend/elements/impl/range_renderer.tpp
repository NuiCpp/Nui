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
            created_ = materialized;
            valueRange_.attachEvent(Nui::globalEventContext.registerEvent(Event{
                [self = this->shared_from_this()](int) -> bool {
                    return self->updateChildren();
                },
                [this /* fine because other function holds this */]() {
                    return !created_.expired();
                },
            }));
            updateChildren();
        }

      protected:
        ObservedType const& valueRange_;
        GeneratorT elementRenderer_;
        std::weak_ptr<Nui::Dom::Element> created_;
    };

    template <typename RangeT, typename GeneratorT>
    class RangeRenderer<RangeT, GeneratorT, true>
        : public BasicObservedRenderer<RangeT, GeneratorT>
        , public std::enable_shared_from_this<RangeRenderer<RangeT, GeneratorT, true>>
    {
      public:
        using BasicObservedRenderer<RangeT, GeneratorT>::BasicObservedRenderer;
        using BasicObservedRenderer<RangeT, GeneratorT>::created_;
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
            auto parent = created_.lock();
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
            created_ = materialized;
            valueRange_.attachEvent(Nui::globalEventContext.registerEvent(Event{
                [self = this->shared_from_this()](int) -> bool {
                    return self->updateChildren();
                },
                [this /* fine because other function holds this */]() {
                    return !created_.expired();
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
        using BasicObservedRenderer<RangeT, GeneratorT>::created_;
        using BasicObservedRenderer<RangeT, GeneratorT>::valueRange_;
        using BasicObservedRenderer<RangeT, GeneratorT>::elementRenderer_;

        bool updateChildren() const override
        {
            auto parent = created_.lock();
            if (!parent)
                return InvalidateRange;

            fullRangeUpdate(parent);
            return KeepRange;
        }
    };

    template <typename IteratorT, typename GeneratorT>
    class BasicStaticRangeRenderer
    {
      public:
        template <typename Generator = GeneratorT>
        BasicStaticRangeRenderer(StaticRange<IteratorT> staticRange, Generator&& elementRenderer)
            : staticRange_{std::move(staticRange)}
            , elementRenderer_{std::forward<GeneratorT>(elementRenderer)}
        {}

        void operator()(auto& materialized) const
        {
            long long counter = 0;
            for (auto& element : staticRange_)
                elementRenderer_(counter++, element)(*materialized, Renderer{.type = RendererType::Append});
        }

      protected:
        StaticRange<IteratorT> staticRange_;
        GeneratorT elementRenderer_;
    };

    template <typename IteratorT, typename GeneratorT>
    class RangeRenderer<StaticRange<IteratorT>, GeneratorT, true>
        : public BasicStaticRangeRenderer<IteratorT, GeneratorT>
    {
      public:
        using BasicStaticRangeRenderer<IteratorT, GeneratorT>::BasicStaticRangeRenderer;
        using BasicStaticRangeRenderer<IteratorT, GeneratorT>::operator();
    };

    template <typename IteratorT, typename GeneratorT>
    class RangeRenderer<StaticRange<IteratorT>, GeneratorT, false>
        : public BasicStaticRangeRenderer<IteratorT, GeneratorT>
    {
      public:
        using BasicStaticRangeRenderer<IteratorT, GeneratorT>::BasicStaticRangeRenderer;
        using BasicStaticRangeRenderer<IteratorT, GeneratorT>::operator();
    };
}