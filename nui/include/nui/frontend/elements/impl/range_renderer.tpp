namespace Nui::Detail
{
    enum RangeStillValid : bool
    {
        InvalidateRange = false,
        KeepRange = true,
    };

    template <typename ObservedType>
    struct HoldToken
    {
        // FIXME: Remove this, when getValueRange overload is not instantiated for non weak observed types
        std::shared_ptr<ObservedType> locked;
    };

    template <typename T>
    struct HoldToken<std::weak_ptr<T>>
    {
        std::shared_ptr<T> locked;
    };

    template <typename T>
    using CommonHoldToken = ::Nui::Detail::HoldToken<ObservedAddMutableReference_raw<typename T::ObservedType>>;

    template <typename RangeT, typename GeneratorT>
    class BasicObservedRenderer
    {
      public:
        using ObservedType = typename RangeT::ObservedType;
        using HoldToken = CommonHoldToken<RangeT>;
        using RendererVector =
            std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>;

        template <typename Generator = GeneratorT>
        BasicObservedRenderer(
            ObservedAddMutableReference_t<ObservedType>&& observed,
            Generator&& elementRenderer,
            RendererVector&& before,
            RendererVector&& after)
            : elementRenderer_{std::forward<GeneratorT>(elementRenderer)}
            , before_{std::move(before)}
            , after_{std::move(after)}
            , valueRange_{std::forward<ObservedAddMutableReference_t<ObservedType>>(observed)}
        {}
        virtual ~BasicObservedRenderer() = default;
        BasicObservedRenderer(BasicObservedRenderer const&) = delete;
        BasicObservedRenderer(BasicObservedRenderer&&) = delete;
        BasicObservedRenderer& operator=(BasicObservedRenderer const&) = delete;
        BasicObservedRenderer& operator=(BasicObservedRenderer&&) = delete;

        /**
         * @brief Handles getting the value range, taking care of weak observed types.
         *
         * @param holder A hold token that keeps locked weak pointers alive.
         * @return auto* A pointer to a Nui::Observed<T, Tags>.
         */
        auto* getValueRange(HoldToken& holder)
        {
            return Nui::overloaded{
                [](::Nui::IsObserved auto& valueRange) {
                    return &valueRange;
                },
                [&holder](::Nui::IsWeakObserved auto& valueRange) {
                    holder.locked = valueRange.lock();
                    // get() yields nullptr when the weak_ptr could not be locked; both
                    // overloads must return the same Observed* type for auto deduction.
                    return holder.locked.get();
                },
            }(valueRange_);
        }

        /**
         * @brief Conditionally renders the entire range anew. Returns whether a full range update was actually
         * performed
         *
         * @param parent The parent element that holds all children.
         * @param force Force a full range update in all cases.
         * @return true A full range update was performed
         * @return false Nothing was done.
         */
        bool fullRangeUpdate(auto& parent, bool force = false)
        {
            auto valueRangeHolder = HoldToken{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (valueRange == nullptr)
                return false;

            if (ownContext().isFullRangeUpdate() || force)
            {
                parent->clearChildren();
                if (!before_.empty())
                    parent->appendElements(before_);
                renderedBeforeCount_ = parent->childCount();

                long long counter = 0;
                for (auto& element : valueRange->value())
                    elementRenderer_(counter++, element)(*parent, Renderer{.type = RendererType::Append});
                ownContext().reset();

                const auto childCountBeforeAfter = parent->childCount();
                if (!after_.empty())
                    parent->appendElements(after_);
                renderedAfterCount_ = parent->childCount() - childCountBeforeAfter;
                return true;
            }
            return false;
        }

        virtual bool updateChildren(bool initial) = 0;

      protected:
        RangeEventContext& ownContext()
        {
            return *ownContext_;
        }
        std::shared_ptr<RangeEventContext> const& ownContextPtr() const
        {
            return ownContext_;
        }
        /// The children that render the rows, without the elements before and after them.
        long renderedRowCount(auto const& parent) const
        {
            return static_cast<long>(parent->childCount()) - static_cast<long>(renderedBeforeCount_) -
                static_cast<long>(renderedAfterCount_);
        }

      protected:
        GeneratorT elementRenderer_;
        std::weak_ptr<Nui::Dom::Element> weakMaterialized_;
        RendererVector before_;
        RendererVector after_;
        std::size_t renderedBeforeCount_{0};
        std::size_t renderedAfterCount_{0};
        std::shared_ptr<RangeEventContext> ownContext_{std::make_shared<RangeEventContext>()};

      private:
        ObservedAddMutableReference_t<ObservedType> valueRange_;
    };

    template <typename RangeT, typename GeneratorT>
    class RangeRenderer<RangeT, GeneratorT, true>
        : public BasicObservedRenderer<RangeT, GeneratorT>
        , public std::enable_shared_from_this<RangeRenderer<RangeT, GeneratorT, true>>
    {
      public:
        using BasicObservedRenderer<RangeT, GeneratorT>::BasicObservedRenderer;
        using BasicObservedRenderer<RangeT, GeneratorT>::weakMaterialized_;
        using BasicObservedRenderer<RangeT, GeneratorT>::elementRenderer_;
        using BasicObservedRenderer<RangeT, GeneratorT>::fullRangeUpdate;
        using BasicObservedRenderer<RangeT, GeneratorT>::getValueRange;
        using BasicObservedRenderer<RangeT, GeneratorT>::ownContext;
        using BasicObservedRenderer<RangeT, GeneratorT>::before_;
        using BasicObservedRenderer<RangeT, GeneratorT>::after_;
        using BasicObservedRenderer<RangeT, GeneratorT>::renderedBeforeCount_;
        using BasicObservedRenderer<RangeT, GeneratorT>::renderedRowCount;

        /// @return false if a range is out of bounds, nothing is rendered from there on.
        bool insertions(auto& parent)
        {
            auto valueRangeHolder = CommonHoldToken<RangeT>{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (valueRange == nullptr)
                return true;

            const auto rowCount = static_cast<long>(valueRange->value().size());
            for (auto i = ownContext().begin(), end = ownContext().end(); i != end; ++i)
            {
                // Inserts are replayed from the front, each one starts at most right after the rows rendered so far.
                if (i->low() < 0 || i->high() >= rowCount || i->low() > renderedRowCount(parent))
                    return false;
                for (auto r = i->low(), high = i->high(); r <= high; ++r)
                {
                    elementRenderer_(r, valueRange->value()[static_cast<std::size_t>(r)])(
                        *parent,
                        Renderer{
                            .type = RendererType::Insert,
                            .metadata = static_cast<std::size_t>(r) + renderedBeforeCount_});
                }
            }
            return true;
        }

        /// @return false if a range is out of bounds, nothing is rendered from there on.
        bool modifications(auto& parent)
        {
            auto valueRangeHolder = CommonHoldToken<RangeT>{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (valueRange == nullptr)
                return true;

            const auto rowCount = static_cast<long>(valueRange->value().size());
            for (auto const& range : ownContext())
            {
                if (range.low() < 0 || range.high() >= rowCount || range.high() >= renderedRowCount(parent))
                    return false;
                for (auto r = range.low(), high = range.high(); r <= high; ++r)
                {
                    elementRenderer_(r, valueRange->value()[static_cast<std::size_t>(r)])(
                        *(*parent)[static_cast<std::size_t>(r) + renderedBeforeCount_],
                        Renderer{.type = RendererType::Replace});
                }
            }
            return true;
        }

        /// @return false if a range is out of bounds, nothing is erased from there on.
        bool erasures(auto& parent)
        {
            auto valueRangeHolder = CommonHoldToken<RangeT>{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (valueRange == nullptr)
                return true;

            for (auto const& eraseRange : reverse_view{ownContext()})
            {
                // An empty range, [low, low - 1], erases nothing.
                if (eraseRange.low() < 0 || eraseRange.high() < eraseRange.low() - 1 ||
                    eraseRange.high() >= renderedRowCount(parent))
                    return false;
                parent->erase(
                    begin(*parent) + eraseRange.low() + renderedBeforeCount_,
                    begin(*parent) + eraseRange.high() + 1 + renderedBeforeCount_);
            }
            return true;
        }

        bool updateChildren(bool initial) override
        {
            auto parent = weakMaterialized_.lock();
            if (!parent)
                return InvalidateRange;

            auto valueRangeHolder = CommonHoldToken<RangeT>{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (!valueRange)
                return InvalidateRange;

            // Regenerate all elements if necessary:
            if (fullRangeUpdate(parent, initial))
                return KeepRange;

            bool inBounds = true;
            switch (ownContext().operationType())
            {
                case RangeOperationType::Insert:
                    inBounds = insertions(parent);
                    break;
                case RangeOperationType::Modify:
                    inBounds = modifications(parent);
                    break;
                case RangeOperationType::Erase:
                    inBounds = erasures(parent);
                    break;
                default:
                    break;
            }
            // Tracked ranges that do not fit the rows or the children would corrupt memory when replayed. Rendering the
            // whole range anew is slower, but correct.
            if (!inBounds)
                fullRangeUpdate(parent, true);

            return KeepRange;
        }

        void operator()(auto& materialized)
        {
            weakMaterialized_ = materialized;

            auto valueRangeHolder = CommonHoldToken<RangeT>{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (valueRange)
            {
                valueRange->attachReaderContext(this->ownContextPtr());
                valueRange->attachEvent(
                    Nui::globalEventContext.registerEvent(
                        Event{
                            [self = this->shared_from_this()](int) -> bool {
                                return self->updateChildren(false);
                            },
                            [this /* fine because other function holds this */]() {
                                return !weakMaterialized_.expired();
                            },
                        }));
                updateChildren(true);
            }
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
        using BasicObservedRenderer<RangeT, GeneratorT>::elementRenderer_;
        using BasicObservedRenderer<RangeT, GeneratorT>::fullRangeUpdate;
        using BasicObservedRenderer<RangeT, GeneratorT>::getValueRange;

        bool updateChildren(bool) override
        {
            auto parent = weakMaterialized_.lock();
            if (!parent)
                return InvalidateRange;

            fullRangeUpdate(parent, true /* force always */);
            return KeepRange;
        }

        void operator()(auto& materialized)
        {
            weakMaterialized_ = materialized;
            auto valueRangeHolder = CommonHoldToken<RangeT>{};
            auto* valueRange = getValueRange(valueRangeHolder);
            if (valueRange)
            {
                valueRange->attachReaderContext(this->ownContextPtr());
                valueRange->attachEvent(
                    Nui::globalEventContext.registerEvent(
                        Event{
                            [self = this->shared_from_this()](int) -> bool {
                                return self->updateChildren(true);
                            },
                            [this /* fine because other function holds this */]() {
                                return !weakMaterialized_.expired();
                            },
                        }));
                updateChildren(true);
            }
        }
    };

    template <typename RangeLike, typename GeneratorT, typename... ObservedT>
    class UnoptimizedRangeRenderer
        : public std::enable_shared_from_this<UnoptimizedRangeRenderer<RangeLike, GeneratorT, ObservedT...>>
    {
      public:
        using RendererVector =
            std::vector<std::function<std::shared_ptr<Dom::Element>(Dom::Element&, Renderer const&)>>;

        template <typename Generator = GeneratorT>
        UnoptimizedRangeRenderer(
            UnoptimizedRange<RangeLike, ObservedT...>&& unoptimizedRange,
            Generator&& elementRenderer)
            : unoptimizedRange_{std::move(unoptimizedRange)}
            , elementRenderer_{std::forward<GeneratorT>(elementRenderer)}
        {}

        bool updateChildren(bool)
        {
            auto materialized = weakMaterialized_.lock();
            if (!materialized)
                return InvalidateRange;

            materialized->clearChildren();
            materialized->appendElements(unoptimizedRange_.before());

            long long counter = 0;
            for (auto&& element : unoptimizedRange_)
            {
                elementRenderer_(counter++, ContainerWrapUtility::unwrapReferenceWrapper(element))(
                    *materialized, Renderer{.type = RendererType::Append});
            }

            materialized->appendElements(unoptimizedRange_.after());

            return KeepRange;
        }

        void operator()(auto& materialized)
        {
            weakMaterialized_ = materialized;
            unoptimizedRange_.underlying().attachEvent(
                Nui::globalEventContext.registerEvent(
                    Event{
                        [self = this->shared_from_this()](int) -> bool {
                            return self->updateChildren(true);
                        },
                        [this]() {
                            return !weakMaterialized_.expired();
                        },
                    }));
            updateChildren(true);
        }

      protected:
        UnoptimizedRange<RangeLike, ObservedT...> unoptimizedRange_;
        GeneratorT elementRenderer_;
        std::weak_ptr<Nui::Dom::Element> weakMaterialized_;
    };
}