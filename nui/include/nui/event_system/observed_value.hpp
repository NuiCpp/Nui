#pragma once

#include <iterator>
#include <nui/concepts.hpp>
#include <nui/event_system/range_event_context.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/utility/assert.hpp>
#include <nui/utility/meta/pick_first.hpp>
#include <nui/utility/move_detector.hpp>

#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <list>
#include <utility>
#include <deque>
#include <string>
#include <cassert>
#include <set>

namespace Nui
{
    struct CustomEventContextFlag_t
    {};
    inline constexpr CustomEventContextFlag_t CustomEventContextFlag{};

    class ObservedBase
    {
      public:
        explicit ObservedBase(CustomEventContextFlag_t, EventContext* ctx)
            : eventContext_{ctx}
            , attachedEvents_{}
            , attachedOneshotEvents_{}
        {}
        virtual ~ObservedBase() = default;
        ObservedBase(ObservedBase const&) = delete;
        ObservedBase(ObservedBase&& other) noexcept
            : eventContext_{other.eventContext_}
            , attachedEvents_{}
            , attachedOneshotEvents_{}
        {
            // events are outside the value logic of the observed class. the contained value is moved, but the events
            // are merged.
            try
            {
                attachedEvents_.reserve(attachedEvents_.size() + other.attachedEvents_.size());
                attachedOneshotEvents_.reserve(attachedOneshotEvents_.size() + other.attachedOneshotEvents_.size());

                for (auto& event : other.attachedEvents_)
                    attachedEvents_.push_back(std::move(event));
                for (auto& event : other.attachedOneshotEvents_)
                    attachedOneshotEvents_.push_back(std::move(event));
            }
            catch (...)
            {
                std::terminate();
            }
        }
        ObservedBase& operator=(ObservedBase const&) = delete;
        ObservedBase& operator=(ObservedBase&& other) noexcept
        {
            eventContext_ = other.eventContext_;
            try
            {
                attachedEvents_.reserve(attachedEvents_.size() + other.attachedEvents_.size());
                attachedOneshotEvents_.reserve(attachedOneshotEvents_.size() + other.attachedOneshotEvents_.size());

                for (auto& event : other.attachedEvents_)
                    attachedEvents_.push_back(std::move(event));
                for (auto& event : other.attachedOneshotEvents_)
                    attachedOneshotEvents_.push_back(std::move(event));
            }
            catch (...)
            {
                std::terminate();
            }

            return *this;
        }

        void attachEvent(EventContext::EventIdType eventId) const
        {
            attachedEvents_.emplace_back(eventId);
        }
        void attachOneshotEvent(EventContext::EventIdType eventId) const
        {
            attachedOneshotEvents_.emplace_back(eventId);
        }
        void detachEvent(EventContext::EventIdType eventId) const
        {
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), eventId),
                std::end(attachedEvents_));
        }

        std::size_t attachedEventCount() const
        {
            return attachedEvents_.size();
        }
        std::size_t attachedOneshotEventCount() const
        {
            return attachedOneshotEvents_.size();
        }
        std::size_t totalAttachedEventCount() const
        {
            return attachedEvents_.size() + attachedOneshotEvents_.size();
        }

        /**
         * @brief You should never need to do this.
         */
        void detachAllEvents()
        {
            attachedEvents_.clear();
            attachedOneshotEvents_.clear();
        }

        virtual void update(bool /*force*/ = false) const
        {
            NUI_ASSERT(eventContext_ != nullptr, "Event context must never be null.");

            for (auto& event : attachedEvents_)
            {
                auto activationResult = eventContext_->activateEvent(event);
                if (activationResult.found == false)
                    event = EventRegistry::invalidEventId;
            }
            for (auto& event : attachedOneshotEvents_)
                eventContext_->activateEvent(event);
            attachedOneshotEvents_.clear();
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), EventRegistry::invalidEventId),
                std::end(attachedEvents_));
        }

        void updateNow(bool force = false) const
        {
            NUI_ASSERT(eventContext_ != nullptr, "Event context must never be null.");

            update(force);
            eventContext_->executeActiveEventsImmediately();
        }

      protected:
        EventContext* eventContext_;
        mutable std::vector<EventContext::EventIdType> attachedEvents_;
        mutable std::vector<EventContext::EventIdType> attachedOneshotEvents_;
    };

    template <typename ContainedT>
    class ModifiableObserved : public ObservedBase
    {
      public:
        using value_type = ContainedT;

      public:
        class ModificationProxy
        {
          public:
            explicit ModificationProxy(ModifiableObserved& observed)
                : observed_{observed}
                , now_{false}
            {}
            explicit ModificationProxy(ModifiableObserved& observed, bool now)
                : observed_{observed}
                , now_{now}
            {}
            ~ModificationProxy()
            {
                try
                {
                    if (now_)
                        observed_.updateNow(true);
                    else
                        observed_.update(true);
                }
                catch (...)
                {
                    // TODO: log?
                }
            }
            auto& value()
            {
                return observed_.contained_;
            }
            auto* operator->()
            {
                return &observed_.contained_;
            }
            auto& operator*()
            {
                return observed_.contained_;
            }
            operator ContainedT&()
            {
                return observed_.contained_;
            }

          private:
            ModifiableObserved& observed_;
            bool now_;
        };

      public:
        ModifiableObserved()
            : ObservedBase{CustomEventContextFlag, &globalEventContext}
            , contained_{}
        {}
        ModifiableObserved(const ModifiableObserved&) = delete;
        ModifiableObserved(ModifiableObserved&& other)
            : ObservedBase{std::move(other)}
            , contained_{std::move(other.contained_)}
        {
            update();
        };
        ModifiableObserved& operator=(const ModifiableObserved&) = delete;
        ModifiableObserved& operator=(ModifiableObserved&& other)
        {
            if (this != &other)
            {
                ObservedBase::operator=(std::move(other));
                contained_ = std::move(other.contained_);
                update();
            }
            return *this;
        };
        ModifiableObserved& operator=(ContainedT const& contained)
        {
            contained_ = contained;
            update();
            return *this;
        }
        ModifiableObserved& operator=(ContainedT&& contained)
        {
            contained_ = std::move(contained);
            update();
            return *this;
        }
        ~ModifiableObserved() = default;

        template <typename T = ContainedT>
        explicit ModifiableObserved(T&& t)
            : ObservedBase{CustomEventContextFlag, &globalEventContext}
            , contained_{std::forward<T>(t)}
        {}

        explicit ModifiableObserved(CustomEventContextFlag_t, EventContext* ctx)
            : ObservedBase{CustomEventContextFlag, ctx}
            , contained_{}
        {}

        template <typename T = ContainedT>
        explicit ModifiableObserved(CustomEventContextFlag_t, EventContext* ctx, T&& t)
            : ObservedBase{CustomEventContextFlag, ctx}
            , contained_{std::forward<T>(t)}
        {}

        /**
         * @brief Assign a completely new value.
         *
         * @param t
         * @return ModifiableObserved&
         */
        template <typename T = ContainedT>
        ModifiableObserved& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            update();
            return *this;
        }

        template <typename T = ContainedT, typename U>
        requires PlusAssignable<T, U>
        ModifiableObserved<T>& operator+=(U const& rhs)
        {
            this->contained_ += rhs;
            return *this;
        }
        template <typename T = ContainedT, typename U>
        requires MinusAssignable<T, U>
        ModifiableObserved<T>& operator-=(U const& rhs)
        {
            this->contained_ -= rhs;
            return *this;
        }

        template <typename T = ContainedT>
        requires std::equality_comparable_with<ContainedT, T> && Fundamental<T> && Fundamental<ContainedT>
        ModifiableObserved& operator=(T&& t)
        {
            return assignChecked(t);
        }

        template <typename T = ContainedT>
        requires std::equality_comparable_with<ContainedT, T>
        ModifiableObserved& assignChecked(T&& other)
        {
            if (contained_ != other)
            {
                contained_ = std::forward<T>(other);
                update();
            }
            return *this;
        }

        /**
         * @brief Can be used to make mutations to the underlying class that get commited when the returned proxy is
         * destroyed.
         *
         * @return ModificationProxy
         */
        ModificationProxy modify()
        {
            return ModificationProxy{*this};
        }

        ModificationProxy modifyNow()
        {
            return ModificationProxy{*this, true};
        }

        explicit operator bool() const
        requires std::convertible_to<ContainedT, bool>
        {
            return static_cast<bool>(contained_);
        }

        ContainedT& value()
        {
            return contained_;
        }
        ContainedT const& value() const
        {
            return contained_;
        }
        ContainedT& operator*()
        {
            return contained_;
        }
        ContainedT const& operator*() const
        {
            return contained_;
        }
        ContainedT* operator->()
        {
            return &contained_;
        }
        ContainedT const* operator->() const
        {
            return &contained_;
        }

        /**
         * @brief Sets the value without making an update.
         */
        void assignWithoutUpdate(ContainedT&& t)
        {
            contained_ = std::forward<ContainedT>(t);
        }

      protected:
        ContainedT contained_;
    };

    template <typename ContainerT>
    class ObservedContainer;

    namespace ContainerWrapUtility
    {
        template <typename T, typename ContainerT>
        class ReferenceWrapper
        {
          public:
            ReferenceWrapper(ObservedContainer<ContainerT>* owner, std::size_t pos, T& ref)
                : owner_{owner}
                , pos_{pos}
                , ref_{&ref}
            {}
            ReferenceWrapper(ReferenceWrapper const&) = default;
            ReferenceWrapper(ReferenceWrapper&& other) noexcept
                : owner_{other.owner_}
                , pos_{other.pos_}
                , ref_{other.ref_}
            {
                other.owner_ = nullptr;
                other.pos_ = 0;
                other.ref_ = nullptr;
            }
            ReferenceWrapper& operator=(ReferenceWrapper const&) = default;
            ReferenceWrapper& operator=(ReferenceWrapper&& other) noexcept
            {
                if (this != &other)
                {
                    owner_ = other.owner_;
                    pos_ = other.pos_;
                    ref_ = other.ref_;
                    other.owner_ = nullptr;
                    other.pos_ = 0;
                    other.ref_ = nullptr;
                }
                return *this;
            }
            operator T&()
            {
                return *ref_;
            }
            T& operator*()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
                return *ref_;
            }
            T const& operator*() const
            {
                return *ref_;
            }
            T* operator->()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
                return ref_;
            }
            T const* operator->() const
            {
                return ref_;
            }
            T& get()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
                return *ref_;
            }
            T const& getReadonly()
            {
                return *ref_;
            }
            void operator=(T&& val)
            {
                *ref_ = std::move(val);
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
            }
            void operator=(T const& val)
            {
                *ref_ = val;
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
            }

          protected:
            ObservedContainer<ContainerT>* owner_;
            std::size_t pos_;
            T* ref_;
        };

        template <typename T, typename ContainerT>
        auto& unwrapReferenceWrapper(ReferenceWrapper<T, ContainerT>& wrapper)
        {
            return wrapper.get();
        }
        template <typename T, typename ContainerT>
        auto const& unwrapReferenceWrapper(ReferenceWrapper<T, ContainerT> const& wrapper)
        {
            return wrapper.get();
        }
        auto& unwrapReferenceWrapper(auto& ref)
        {
            return ref;
        }
        auto const& unwrapReferenceWrapper(auto const& ref)
        {
            return ref;
        }

        template <typename T, typename ContainerT>
        class PointerWrapper
        {
          public:
            PointerWrapper(ObservedContainer<ContainerT>* owner, std::size_t pos, T* ptr) noexcept
                : owner_{owner}
                , pos_{pos}
                , ptr_{ptr}
            {}
            operator T&()
            {
                return *ptr_;
            }
            T& operator*()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
                return *ptr_;
            }
            T const& operator*() const
            {
                return *ptr_;
            }
            T* operator->()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
                return ptr_;
            }
            T const* operator->() const
            {
                return ptr_;
            }
            T& get()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
                return *ptr_;
            }
            T const& getReadonly()
            {
                return *ptr_;
            }
            void operator=(T* ptr)
            {
                ptr_ = ptr;
                owner_->insertRangeChecked(pos_, pos_, RangeOperationType::Modify);
            }

          protected:
            ObservedContainer<ContainerT>* owner_;
            std::size_t pos_;
            T* ptr_;
        };

        template <typename WrappedIterator, typename ContainerT>
        class IteratorWrapper
        {
          public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = typename WrappedIterator::value_type;
            using difference_type = typename WrappedIterator::difference_type;
            using pointer = PointerWrapper<value_type, ContainerT>;
            using reference = ReferenceWrapper<value_type, ContainerT>;

          public:
            IteratorWrapper(ObservedContainer<ContainerT>* owner, WrappedIterator it)
                : owner_{owner}
                , it_{std::move(it)}
            {}
            IteratorWrapper(IteratorWrapper const&) = default;
            IteratorWrapper(IteratorWrapper&&) = default;
            IteratorWrapper& operator=(IteratorWrapper const&) = default;
            IteratorWrapper& operator=(IteratorWrapper&&) = default;
            IteratorWrapper& operator+=(difference_type n)
            {
                it_ += n;
                return *this;
            }
            IteratorWrapper& operator-=(difference_type n)
            {
                it_ -= n;
                return *this;
            }
            IteratorWrapper& operator++()
            {
                ++it_;
                return *this;
            }
            IteratorWrapper operator++(int)
            {
                return IteratorWrapper{it_++};
            }
            IteratorWrapper& operator--()
            {
                --it_;
                return *this;
            }
            IteratorWrapper operator--(int)
            {
                return IteratorWrapper{it_--};
            }
            friend IteratorWrapper operator+(IteratorWrapper const& wrap, difference_type n)
            {
                return IteratorWrapper{wrap.owner_, wrap.it_ + n};
            }
            friend IteratorWrapper operator-(IteratorWrapper const& wrap, difference_type n)
            {
                return IteratorWrapper{wrap.owner_, wrap.it_ - n};
            }
            difference_type operator-(IteratorWrapper const& other) const
            {
                return it_ - other.it_;
            }
            auto operator*()
            {
                if constexpr (std::is_same_v<WrappedIterator, typename ContainerT::reverse_iterator>)
                    return ReferenceWrapper<value_type, ContainerT>{
                        owner_,
                        owner_->contained_.size() - static_cast<std::size_t>(1) -
                            static_cast<std::size_t>(it_ - owner_->contained_.rbegin()),
                        *it_};
                else
                    return ReferenceWrapper<value_type, ContainerT>{
                        owner_, static_cast<std::size_t>(it_ - owner_->contained_.begin()), *it_};
            }
            auto operator*() const
            {
                return *it_;
            }
            auto operator->()
            {
                if constexpr (std::is_same_v<WrappedIterator, typename ContainerT::reverse_iterator>)
                    return PointerWrapper<value_type, ContainerT>{
                        owner_,
                        owner_->contained_.size() - static_cast<std::size_t>(1) -
                            static_cast<std::size_t>(it_ - owner_->contained_.rbegin()),
                        &*it_};
                else
                    return PointerWrapper<value_type, ContainerT>{
                        owner_, static_cast<std::size_t>(it_ - owner_->contained_.begin()), &*it_};
            }
            auto operator->() const
            {
                return &*it_;
            }
            IteratorWrapper operator[](std::size_t offset) const
            {
                return IteratorWrapper{owner_, it_[offset]};
            }
            bool operator<(IteratorWrapper const& other) const
            {
                return it_ < other.it_;
            }
            bool operator>(IteratorWrapper const& other) const
            {
                return it_ > other.it_;
            }
            bool operator<=(IteratorWrapper const& other) const
            {
                return it_ <= other.it_;
            }
            bool operator>=(IteratorWrapper const& other) const
            {
                return it_ >= other.it_;
            }
            bool operator==(IteratorWrapper const& other) const
            {
                return it_ == other.it_;
            }
            WrappedIterator getWrapped() const
            {
                return it_;
            }

          private:
            ObservedContainer<ContainerT>* owner_;
            WrappedIterator it_;
        };
    };

    template <typename ContainerT>
    class ObservedContainer : public ModifiableObserved<ContainerT>
    {
      public:
        friend class ContainerWrapUtility::ReferenceWrapper<typename ContainerT::value_type, ContainerT>;
        friend class ContainerWrapUtility::PointerWrapper<typename ContainerT::value_type, ContainerT>;

        using value_type = typename ContainerT::value_type;
        using allocator_type = typename ContainerT::allocator_type;
        using size_type = typename ContainerT::size_type;
        using difference_type = typename ContainerT::difference_type;
        using reference = ContainerWrapUtility::ReferenceWrapper<typename ContainerT::value_type, ContainerT>;
        using const_reference = typename ContainerT::const_reference;
        using pointer = ContainerWrapUtility::PointerWrapper<typename ContainerT::value_type, ContainerT>;
        using const_pointer = typename ContainerT::const_pointer;

        using iterator = ContainerWrapUtility::IteratorWrapper<typename ContainerT::iterator, ContainerT>;
        using const_iterator = typename ContainerT::const_iterator;
        using reverse_iterator =
            ContainerWrapUtility::IteratorWrapper<typename ContainerT::reverse_iterator, ContainerT>;
        using const_reverse_iterator = typename ContainerT::const_reverse_iterator;

        using ModifiableObserved<ContainerT>::contained_;

      public:
        explicit ObservedContainer(CustomEventContextFlag_t, EventContext* ctx)
            : ModifiableObserved<ContainerT>{CustomEventContextFlag, ctx}
            , rangeContext_{std::make_shared<RangeEventContext>()}
            , afterEffectId_{registerAfterEffect()}
        {}
        explicit ObservedContainer()
            : ModifiableObserved<ContainerT>{}
            , rangeContext_{std::make_shared<RangeEventContext>()}
            , afterEffectId_{registerAfterEffect()}
        {}
        template <typename T = ContainerT>
        explicit ObservedContainer(CustomEventContextFlag_t, EventContext* ctx, T&& t)
            : ModifiableObserved<ContainerT>{CustomEventContextFlag, ctx, std::forward<T>(t)}
            , rangeContext_{std::make_shared<RangeEventContext>()}
            , afterEffectId_{registerAfterEffect()}
        {}
        template <typename T = ContainerT>
        explicit ObservedContainer(T&& t)
            : ModifiableObserved<ContainerT>{std::forward<T>(t)}
            , rangeContext_{std::make_shared<RangeEventContext>()}
            , afterEffectId_{registerAfterEffect()}
        {}
        explicit ObservedContainer(RangeEventContext&& rangeContext)
            : ModifiableObserved<ContainerT>{}
            , rangeContext_{std::make_shared<RangeEventContext>(std::move(rangeContext))}
            , afterEffectId_{registerAfterEffect()}
        {}
        explicit ObservedContainer(CustomEventContextFlag_t, EventContext* ctx, RangeEventContext&& rangeContext)
            : ModifiableObserved<ContainerT>{CustomEventContextFlag, ctx}
            , rangeContext_{std::make_shared<RangeEventContext>(std::move(rangeContext))}
            , afterEffectId_{registerAfterEffect()}
        {}
        template <typename T = ContainerT>
        ObservedContainer(T&& t, RangeEventContext&& rangeContext)
            : ModifiableObserved<ContainerT>{std::forward<T>(t)}
            , rangeContext_{std::make_shared<RangeEventContext>(std::move(rangeContext))}
            , afterEffectId_{registerAfterEffect()}
        {}
        template <typename T = ContainerT>
        ObservedContainer(CustomEventContextFlag_t, EventContext* ctx, T&& t, RangeEventContext&& rangeContext)
            : ModifiableObserved<ContainerT>{CustomEventContextFlag, ctx, std::forward<T>(t)}
            , rangeContext_{std::make_shared<RangeEventContext>(std::move(rangeContext))}
            , afterEffectId_{registerAfterEffect()}
        {}

        ObservedContainer(const ObservedContainer&) = delete;
        ObservedContainer(ObservedContainer&&) = default;
        ObservedContainer& operator=(const ObservedContainer&) = delete;
        ObservedContainer& operator=(ObservedContainer&&) = default;
        ~ObservedContainer()
        {
            if (!moveDetector_.wasMoved())
                ObservedBase::eventContext_->removeAfterEffect(afterEffectId_);
        }

        constexpr auto map(auto&& function) const;
        constexpr auto map(auto&& function);

        template <typename T = ContainerT>
        ObservedContainer& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            rangeContext_->reset(true);
            update();
            return *this;
        }
        void assign(size_type count, const value_type& value)
        {
            contained_.assign(count, value);
            rangeContext_->reset(true);
            update();
        }
        template <typename Iterator>
        void assign(Iterator first, Iterator last)
        {
            contained_.assign(first, last);
            rangeContext_->reset(true);
            update();
        }
        void assign(std::initializer_list<value_type> ilist)
        {
            contained_.assign(ilist);
            rangeContext_->reset(true);
            update();
        }

        // Element access
        reference front()
        {
            return reference{this, 0, contained_.front()};
        }
        const_reference front() const
        {
            return contained_.front();
        }
        reference back()
        {
            return reference{this, contained_.size() - 1, contained_.back()};
        }
        const_reference back() const
        {
            return contained_.back();
        }
        pointer data() noexcept
        {
            return pointer{this, 0, contained_.data()};
        }
        const_pointer data() const noexcept
        {
            return contained_.data();
        }
        reference at(size_type pos)
        {
            return reference{this, pos, contained_.at(pos)};
        }
        const_reference at(size_type pos) const
        {
            return contained_.at(pos);
        }
        reference operator[](size_type pos)
        {
            return reference{this, pos, contained_[pos]};
        }
        const_reference operator[](size_type pos) const
        {
            return contained_[pos];
        }

        // Iterators
        iterator begin() noexcept
        {
            return iterator{this, contained_.begin()};
        }
        const_iterator begin() const noexcept
        {
            return contained_.begin();
        }
        iterator end() noexcept
        {
            return iterator{this, contained_.end()};
        }
        const_iterator end() const noexcept
        {
            return contained_.end();
        }
        const_iterator cbegin() const noexcept
        {
            return contained_.cbegin();
        }
        const_iterator cend() const noexcept
        {
            return contained_.cend();
        }
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator{this, contained_.rbegin()};
        }
        const_reverse_iterator rbegin() const noexcept
        {
            return contained_.rbegin();
        }
        reverse_iterator rend() noexcept
        {
            return reverse_iterator{this, contained_.rend()};
        }
        const_reverse_iterator rend() const noexcept
        {
            return contained_.rend();
        }
        const_reverse_iterator crbegin() const noexcept
        {
            return contained_.crbegin();
        }
        const_reverse_iterator crend() const noexcept
        {
            return contained_.crend();
        }

        // Capacity
        bool empty() const noexcept
        {
            return contained_.empty();
        }
        std::size_t size() const noexcept
        {
            return contained_.size();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<std::size_t, decltype(std::declval<U>().max_size())> max_size() const noexcept
        {
            return contained_.max_size();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().reserve(std::declval<std::size_t>()))>
        reserve(size_type capacity)
        {
            return contained_.reserve(capacity);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<std::size_t, decltype(std::declval<U>().capacity())> capacity() const noexcept
        {
            return contained_.capacity();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().shrink_to_fit())> shrink_to_fit()
        {
            return contained_.shrink_to_fit();
        }

        // Modifiers
        void clear()
        {
            contained_.clear();
            rangeContext_->reset(true);
            update();
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<
            std::pair<typename ContainerT::iterator, bool>,
            decltype(std::declval<U>().insert(std::declval<const value_type&>()))>
        insert(const value_type& value)
        {
            NUI_ASSERT(ObservedBase::eventContext_ != nullptr, "Event context must never be null.");

            const auto result = contained_.insert(value);
            rangeContext_->performFullRangeUpdate();
            update();
            ObservedBase::eventContext_->executeActiveEventsImmediately();
            return result;
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<
            std::pair<typename ContainerT::iterator, bool>,
            decltype(std::declval<U>().insert(std::declval<value_type&&>()))>
        insert(value_type&& value)
        {
            NUI_ASSERT(ObservedBase::eventContext_ != nullptr, "Event context must never be null.");

            const auto result = contained_.insert(std::move(value));
            rangeContext_->performFullRangeUpdate();
            update();
            ObservedBase::eventContext_->executeActiveEventsImmediately();
            return result;
        }
        iterator insert(iterator pos, const value_type& value)
        {
            return insert(pos.getWrapped(), value);
        }
        iterator insert(const_iterator pos, const value_type& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, value);
            insertRangeChecked(distance, distance, RangeOperationType::Insert);
            return iterator{this, it};
        }
        iterator insert(iterator pos, value_type&& value)
        {
            return insert(pos.getWrapped(), std::move(value));
        }
        iterator insert(const_iterator pos, value_type&& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, std::move(value));
            insertRangeChecked(distance, distance, RangeOperationType::Insert);
            return iterator{this, it};
        }
        iterator insert(iterator pos, size_type count, const value_type& value)
        {
            return insert(pos.getWrapped(), count, value);
        }
        iterator insert(const_iterator pos, size_type count, const value_type& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, count, value);
            insertRangeChecked(distance, distance + count - 1, RangeOperationType::Insert);
            return iterator{this, it};
        }
        template <typename Iterator>
        iterator insert(iterator pos, Iterator first, Iterator last)
        {
            return insert(pos.getWrapped(), first, last);
        }
        template <typename Iterator>
        iterator insert(const_iterator pos, Iterator first, Iterator last)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, first, last);
            insertRangeChecked(distance, distance + std::distance(first, last) - 1, RangeOperationType::Insert);
            return iterator{this, it};
        }
        iterator insert(iterator pos, std::initializer_list<value_type> ilist)
        {
            return insert(pos.getWrapped(), ilist);
        }
        iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, ilist);
            insertRangeChecked(distance, distance + ilist.size() - 1, RangeOperationType::Insert);
            return iterator{this, it};
        }
        template <typename... Args>
        iterator emplace(const_iterator pos, Args&&... args)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.emplace(pos, std::forward<Args>(args)...);
            insertRangeChecked(distance, distance, RangeOperationType::Insert);
            return iterator{this, it};
        }
        iterator erase(iterator pos)
        {
            const auto distance = pos - begin();
            eraseNotify(distance, distance);
            auto it = contained_.erase(pos.getWrapped());
            insertRangeChecked(distance, distance, RangeOperationType::Erase);
            return iterator{this, it};
        }
        iterator erase(const_iterator pos)
        {
            const auto distance = pos - cbegin();
            eraseNotify(distance, distance);
            auto it = contained_.erase(pos);
            insertRangeChecked(distance, distance, RangeOperationType::Erase);
            return iterator{this, it};
        }
        iterator erase(iterator first, iterator last)
        {
            const auto distance = first - begin();
            const auto distance2 = std::distance(first, last);
            eraseNotify(distance, distance + distance2 - 1);
            auto it = contained_.erase(first.getWrapped(), last.getWrapped());
            insertRangeChecked(distance, distance + distance2 - 1, RangeOperationType::Erase);
            return iterator{this, it};
        }
        iterator erase(const_iterator first, const_iterator last)
        {
            const auto distance = first - cbegin();
            const auto distance2 = std::distance(first, last);
            eraseNotify(distance, distance + distance2 - 1);
            auto it = contained_.erase(first, last);
            insertRangeChecked(distance, distance + distance2 - 1, RangeOperationType::Erase);
            return iterator{this, it};
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_back(std::declval<const value_type&>()))>
        push_back(const value_type& value)
        {
            contained_.push_back(value);
            insertRangeChecked(size() - 1, size() - 1, RangeOperationType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_back(std::declval<value_type>()))>
        push_back(value_type&& value)
        {
            contained_.push_back(std::move(value));
            insertRangeChecked(size() - 1, size() - 1, RangeOperationType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_front(std::declval<const value_type&>()))>
        push_front(const value_type& value)
        {
            contained_.push_front(value);
            insertRangeChecked(0, 0, RangeOperationType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_front(std::declval<value_type>()))>
        push_front(value_type&& value)
        {
            contained_.push_front(std::move(value));
            insertRangeChecked(0, 0, RangeOperationType::Insert);
        }
        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            contained_.emplace_back(std::forward<Args>(args)...);
            insertRangeChecked(size() - 1, size() - 1, RangeOperationType::Insert);
        }
        template <typename U = ContainerT, typename... Args>
        Detail::PickFirst_t<void, decltype(std::declval<U>().emplace_front())> emplace_front(Args&&... args)
        {
            contained_.emplace_front(std::forward<Args>(args)...);
            insertRangeChecked(0, 0, RangeOperationType::Insert);
        }
        void pop_back()
        {
            if (contained_.empty())
                return;
            eraseNotify(size() - 1, size() - 1);
            contained_.pop_back();
            insertRangeChecked(size(), size(), RangeOperationType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().pop_front())> pop_front()
        {
            if (contained_.empty())
                return;
            eraseNotify(0, 0);
            contained_.pop_front();
            insertRangeChecked(0, 0, RangeOperationType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().resize(std::declval<std::size_t>()))>
        resize(size_type count)
        {
            const auto sizeBefore = contained_.size();
            if (sizeBefore < count && sizeBefore != 0)
            {
                eraseNotify(sizeBefore, count - 1);
            }
            contained_.resize(count);
            if (sizeBefore < count)
            {
                insertRangeChecked(sizeBefore, count - 1, RangeOperationType::Insert);
            }
            else if (sizeBefore != 0)
            {
                insertRangeChecked(count, sizeBefore - 1, RangeOperationType::Erase);
            }
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<
            void,
            decltype(std::declval<U>().resize(std::declval<std::size_t>(), std::declval<value_type const&>()))>
        resize(size_type count, value_type const& fillValue)
        {
            const auto sizeBefore = contained_.size();
            eraseNotify(sizeBefore, count - 1);
            contained_.resize(count, fillValue);
            if (sizeBefore < count)
            {
                insertRangeChecked(sizeBefore, count - 1, RangeOperationType::Insert);
            }
            else if (sizeBefore != 0)
            {
                insertRangeChecked(count, sizeBefore - 1, RangeOperationType::Erase);
            }
        }
        void swap(ContainerT& other)
        {
            contained_.swap(other);
            rangeContext_->reset(true);
            update();
        }

        // Other
        ContainerT& value()
        {
            return contained_;
        }
        ContainerT const& value() const
        {
            return contained_;
        }
        RangeEventContext& rangeContext()
        {
            return *rangeContext_;
        }
        RangeEventContext const& rangeContext() const
        {
            return *rangeContext_;
        }

      protected:
        void update(bool force = false) const override
        {
            if (force)
                rangeContext_->reset(true);
            ObservedBase::eventContext_->activateAfterEffect(afterEffectId_);
            ObservedBase::update(force);
        }

      protected:
        void insertRangeChecked(std::size_t low, std::size_t high, RangeOperationType type)
        {
            std::function<void(int)> doInsert;
            doInsert = [&](int retries) {
                NUI_ASSERT(ObservedBase::eventContext_ != nullptr, "Event context must never be null.");

                const auto result = rangeContext_->insertModificationRange(low, high, type);
                if (result == RangeEventContext::InsertResult::Perform)
                {
                    update();
                    ObservedBase::eventContext_->executeActiveEventsImmediately();
                }
                else if (result == RangeEventContext::InsertResult::PerformAndRetry)
                {
                    update();
                    ObservedBase::eventContext_->executeActiveEventsImmediately();

                    if (retries < 3)
                        doInsert(retries + 1);
                    else
                    {
                        rangeContext_->reset(true);
                        update();
                        ObservedBase::eventContext_->executeActiveEventsImmediately();
                        return;
                    }
                }
                else if (result == RangeEventContext::InsertResult::Accepted)
                {
                    update();
                }
                else
                {
                    // Rejected! (why?)
                    rangeContext_->reset(true);
                    update();
                    ObservedBase::eventContext_->executeActiveEventsImmediately();
                    return;
                }
            };

            doInsert(0);
        }

        auto registerAfterEffect()
        {
            NUI_ASSERT(ObservedBase::eventContext_ != nullptr, "Event context must never be null.");
            return ObservedBase::eventContext_->registerAfterEffect(
                Event{[weak = std::weak_ptr<RangeEventContext>{rangeContext_}](EventContext::EventIdType) {
                    if (auto shared = weak.lock(); shared)
                    {
                        shared->reset();
                        return true;
                    }
                    return false;
                }});
        }

        void eraseNotify(std::size_t index, std::size_t high)
        {
            const bool fixupPerformed = rangeContext_->eraseNotify(index, high);
            if (fixupPerformed) // FORCE update:
            {
                update();
                ObservedBase::eventContext_->executeActiveEventsImmediately();
            }
        }

      protected:
        MoveDetector moveDetector_;
        mutable std::shared_ptr<RangeEventContext> rangeContext_;
        mutable EventContext::EventIdType afterEffectId_;
    };

    template <typename T>
    class Observed : public ModifiableObserved<T>
    {
      public:
        using ModifiableObserved<T>::ModifiableObserved;
        using ModifiableObserved<T>::operator=;
        using ModifiableObserved<T>::operator->;

        Observed& operator=(T const& contained)
        {
            ModifiableObserved<T>::operator=(contained);
            return *this;
        }
        Observed& operator=(T&& contained)
        {
            ModifiableObserved<T>::operator=(std::move(contained));
            return *this;
        }
    };
    template <typename... Parameters>
    class Observed<std::vector<Parameters...>> : public ObservedContainer<std::vector<Parameters...>>
    {
      public:
        using ObservedContainer<std::vector<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::vector<Parameters...>>::operator=;
        using ObservedContainer<std::vector<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = true;

        Observed<std::vector<Parameters...>>& operator=(std::vector<Parameters...> const& contained)
        {
            ObservedContainer<std::vector<Parameters...>>::operator=(contained);
            return *this;
        }
        Observed<std::vector<Parameters...>>& operator=(std::vector<Parameters...>&& contained)
        {
            ObservedContainer<std::vector<Parameters...>>::operator=(std::move(contained));
            return *this;
        }
    };
    template <typename... Parameters>
    class Observed<std::deque<Parameters...>> : public ObservedContainer<std::deque<Parameters...>>
    {
      public:
        using ObservedContainer<std::deque<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::deque<Parameters...>>::operator=;
        using ObservedContainer<std::deque<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = true;

        Observed<std::deque<Parameters...>>& operator=(std::deque<Parameters...> const& contained)
        {
            ObservedContainer<std::deque<Parameters...>>::operator=(contained);
            return *this;
        }
        Observed<std::deque<Parameters...>>& operator=(std::deque<Parameters...>&& contained)
        {
            ObservedContainer<std::deque<Parameters...>>::operator=(std::move(contained));
            return *this;
        }
    };
    template <typename... Parameters>
    class Observed<std::basic_string<Parameters...>> : public ObservedContainer<std::basic_string<Parameters...>>
    {
      public:
        using ObservedContainer<std::basic_string<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::basic_string<Parameters...>>::operator=;
        using ObservedContainer<std::basic_string<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = true;

        Observed<std::basic_string<Parameters...>>& operator=(std::basic_string<Parameters...> const& contained)
        {
            ObservedContainer<std::basic_string<Parameters...>>::operator=(contained);
            return *this;
        }
        Observed<std::basic_string<Parameters...>>& operator=(std::basic_string<Parameters...>&& contained)
        {
            ObservedContainer<std::basic_string<Parameters...>>::operator=(std::move(contained));
            return *this;
        }

        Observed<std::basic_string<Parameters...>>& erase(std::size_t index = 0, std::size_t count = std::string::npos)
        {
            if (count == std::size_t{0})
                return *this;
            const auto sizeBefore = this->contained_.size();
            const auto high = count == std::string::npos ? sizeBefore - 1 : count - 1;
            this->eraseNotify(index, high);
            this->contained_.erase(index, count);
            this->insertRangeChecked(index, high, RangeOperationType::Erase);
            return *this;
        }
    };
    template <typename... Parameters>
    class Observed<std::set<Parameters...>> : public ObservedContainer<std::set<Parameters...>>
    {
      public:
        using ObservedContainer<std::set<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::set<Parameters...>>::operator=;
        using ObservedContainer<std::set<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = false;

      public:
        Observed()
            : ObservedContainer<std::set<Parameters...>>{RangeEventContext{true}}
        {}
        template <typename T = std::set<Parameters...>>
        explicit Observed(T&& t)
            : ObservedContainer<std::set<Parameters...>>{std::forward<T>(t), RangeEventContext{true}}
        {}

        Observed<std::set<Parameters...>>& operator=(std::set<Parameters...> const& contained)
        {
            ObservedContainer<std::set<Parameters...>>::operator=(contained);
            return *this;
        }
        Observed<std::set<Parameters...>>& operator=(std::set<Parameters...>&& contained)
        {
            ObservedContainer<std::set<Parameters...>>::operator=(std::move(contained));
            return *this;
        }
    };
    template <typename... Parameters>
    class Observed<std::list<Parameters...>> : public ObservedContainer<std::list<Parameters...>>
    {
      public:
        using ObservedContainer<std::list<Parameters...>>::ObservedContainer;
        using ObservedContainer<std::list<Parameters...>>::operator=;
        using ObservedContainer<std::list<Parameters...>>::operator->;
        static constexpr auto isRandomAccess = false;

      public:
        Observed()
            : ObservedContainer<std::list<Parameters...>>{RangeEventContext{true}}
        {}
        template <typename T = std::list<Parameters...>>
        explicit Observed(T&& t)
            : ObservedContainer<std::list<Parameters...>>{std::forward<T>(t), RangeEventContext{true}}
        {}

        Observed<std::list<Parameters...>>& operator=(std::list<Parameters...> const& contained)
        {
            ObservedContainer<std::list<Parameters...>>::operator=(contained);
            return *this;
        }
        Observed<std::list<Parameters...>>& operator=(std::list<Parameters...>&& contained)
        {
            ObservedContainer<std::list<Parameters...>>::operator=(std::move(contained));
            return *this;
        }
    };

    template <>
    class Observed<void> : public ObservedBase
    {
      public:
        using ObservedBase::ObservedBase;
        using ObservedBase::operator=;

        void modify() const
        {
            update();
        };

        void modifyNow() const
        {
            updateNow();
        };
    };

    namespace Detail
    {
        template <typename T>
        struct IsObserved
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct IsObserved<Observed<T>>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        struct IsWeakObserved
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct IsWeakObserved<std::weak_ptr<Observed<T>>>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        struct IsSharedObserved
        {
            static constexpr bool value = false;
        };

        template <typename T>
        struct IsSharedObserved<std::shared_ptr<Observed<T>>>
        {
            static constexpr bool value = true;
        };

        template <typename T>
        struct IsObservedLike
        {
            static constexpr bool value =
                IsObserved<T>::value || IsWeakObserved<T>::value || IsSharedObserved<T>::value;
        };
    }

    template <typename T>
    requires Incrementable<T>
    inline ModifiableObserved<T>& operator++(ModifiableObserved<T>& observedValue)
    {
        ++observedValue.value();
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    requires Incrementable<T>
    inline T operator++(ModifiableObserved<T>& observedValue, int)
    {
        auto tmp = observedValue.value();
        ++observedValue.value();
        observedValue.update();
        return tmp;
    }

    template <typename T>
    inline auto operator--(ModifiableObserved<T>& observedValue)
        -> ModifiableObserved<Detail::PickFirst_t<T, decltype(--std::declval<T>())>>&
    {
        --observedValue.value();
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    inline auto operator--(ModifiableObserved<T>& observedValue, int)
        -> Detail::PickFirst_t<T, decltype(std::declval<T>()--)>
    {
        auto tmp = observedValue.value();
        --observedValue.value();
        observedValue.update();
        return tmp;
    }

    template <typename T>
    concept IsObserved = Detail::IsObserved<std::decay_t<T>>::value;
    template <typename T>
    concept IsSharedObserved = Detail::IsSharedObserved<std::decay_t<T>>::value;
    template <typename T>
    concept IsWeakObserved = Detail::IsWeakObserved<std::decay_t<T>>::value;
    template <typename T>
    concept IsObservedLike = Detail::IsObservedLike<std::decay_t<T>>::value;

    namespace Detail
    {
        template <typename T>
        struct CopyableObservedWrap // minimal wrapper to make Observed<T> copiable
        {
          public:
            explicit constexpr CopyableObservedWrap(Observed<T> const& observed)
                : observed_{&observed}
            {}

            inline T const& value() const
            {
                return observed_->value();
            }

            inline void attachEvent(auto eventId) const
            {
                observed_->attachEvent(eventId);
            }

            inline void detachEvent(auto eventId) const
            {
                observed_->detachEvent(eventId);
            }

          private:
            Observed<T> const* observed_;
        };

        template <typename T>
        struct ObservedAddReference
        {
            using type = T const&;
        };
        template <>
        struct ObservedAddReference<void>
        {
            using type = void;
        };
        template <typename T>
        struct ObservedAddReference<std::weak_ptr<Observed<T>>>
        {
            using type = std::weak_ptr<Observed<T>>;
        };
        template <typename T>
        struct ObservedAddReference<std::shared_ptr<Observed<T>>>
        {
            using type = std::weak_ptr<Observed<T>>;
        };
        template <typename T>
        struct ObservedAddReference<std::weak_ptr<const Observed<T>>>
        {
            using type = std::weak_ptr<const Observed<T>>;
        };
        template <typename T>
        struct ObservedAddReference<std::shared_ptr<const Observed<T>>>
        {
            using type = std::weak_ptr<const Observed<T>>;
        };

        template <typename T>
        struct ObservedAddMutableReference
        {
            using type = T&;
            using raw = T;
        };
        template <>
        struct ObservedAddMutableReference<void>
        {
            using type = void;
            using raw = void;
        };
        template <typename T>
        struct ObservedAddMutableReference<std::weak_ptr<Observed<T>>>
        {
            using type = std::weak_ptr<Observed<T>>;
            using raw = Observed<T>;
        };
        template <typename T>
        struct ObservedAddMutableReference<std::shared_ptr<Observed<T>>>
        {
            using type = std::weak_ptr<Observed<T>>;
            using raw = Observed<T>;
        };

        template <typename T>
        using ObservedAddReference_t = typename ObservedAddReference<std::decay_t<T>>::type;
        template <typename T>
        using ObservedAddMutableReference_t = typename ObservedAddMutableReference<std::remove_reference_t<T>>::type;
        template <typename T>
        using ObservedAddMutableReference_raw = typename ObservedAddMutableReference<std::remove_reference_t<T>>::raw;
    }

    template <typename T>
    struct UnpackObserved
    {
        using type = T;
    };
    template <typename T>
    struct UnpackObserved<Observed<T>>
    {
        using type = T;
    };
    template <typename T>
    struct UnpackObserved<std::weak_ptr<Observed<T>>>
    {
        using type = T;
    };
    template <typename T>
    struct UnpackObserved<std::shared_ptr<Observed<T>>>
    {
        using type = T;
    };
    template <typename T>
    struct UnpackObserved<std::weak_ptr<const Observed<T>>>
    {
        using type = T;
    };
    template <typename T>
    struct UnpackObserved<std::shared_ptr<const Observed<T>>>
    {
        using type = T;
    };
    template <typename T>
    using UnpackObserved_t = typename UnpackObserved<T>::type;
}