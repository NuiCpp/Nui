#pragma once

#include <iterator>
#include <nui/concepts.hpp>
#include <nui/event_system/range_event_context.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/utility/meta/pick_first.hpp>

#include <memory>
#include <vector>
#include <functional>
#include <type_traits>
#include <list>
#include <utility>
#include <deque>

namespace Nui
{
    class ObservedBase
    {
      public:
        ObservedBase() = default;
        virtual ~ObservedBase() = default;

        void attachEvent(EventContext::EventIdType eventId)
        {
            attachedEvents_.emplace_back(eventId);
        }
        void attachOneshotEvent(EventContext::EventIdType eventId)
        {
            attachedOneshotEvents_.emplace_back(eventId);
        }
        void unattachEvent(EventContext::EventIdType eventId)
        {
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), eventId),
                std::end(attachedEvents_));
        }

      protected:
        void update()
        {
            for (auto& event : attachedEvents_)
            {
                if (globalEventContext.activateEvent(event) == nullptr)
                    event = EventRegistry::invalidEventId;
            }
            for (auto& event : attachedOneshotEvents_)
                globalEventContext.activateEvent(event);
            attachedOneshotEvents_.clear();
            attachedEvents_.erase(
                std::remove(std::begin(attachedEvents_), std::end(attachedEvents_), EventRegistry::invalidEventId),
                std::end(attachedEvents_));
        }

      private:
        std::vector<EventContext::EventIdType> attachedEvents_;
        std::vector<EventContext::EventIdType> attachedOneshotEvents_;
    };

    template <typename ContainedT>
    class Observed : public ObservedBase
    {
      public:
        class ModificationProxy
        {
          public:
            explicit ModificationProxy(Observed& observed)
                : observed_{observed}
            {}
            ~ModificationProxy()
            {
                try
                {
                    observed_.update();
                }
                catch (...)
                {
                    // TODO: log?
                }
            }
            auto& data()
            {
                return observed_.contained_;
            }
            auto* operator->()
            {
                return &observed_.contained_;
            }

          private:
            Observed& observed_;
        };

      public:
        Observed() = default;
        Observed(const Observed&) = delete;
        Observed(Observed&&) = default;
        Observed& operator=(const Observed&) = delete;
        Observed& operator=(Observed&&) = default;
        ~Observed() = default;

        template <typename T = ContainedT>
        Observed(T&& t)
            : contained_{std::forward<T>(t)}
        {}

        /**
         * @brief Assign a completely new value.
         *
         * @param t
         * @return Observed&
         */
        template <typename T = ContainedT>
        Observed& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            update();
            return *this;
        }

        template <typename T>
        requires Incrementable<T>
        friend Observed<T>& operator++(Observed<T>& observedValue);

        template <typename T>
        requires Incrementable<T>
        friend T operator++(Observed<T>& observedValue, int);

        template <typename T>
        requires Decrementable<T>
        friend Observed<T>& operator--(Observed<T>& observedValue);

        template <typename T>
        requires Decrementable<T>
        friend T operator--(Observed<T>& observedValue, int);

        template <typename T = ContainedT, typename U>
        requires PlusAssignable<T, U> Observed<T>
        &operator+=(U const& rhs)
        {
            this->contained_ += rhs;
            return *this;
        }
        template <typename T = ContainedT, typename U>
        requires MinusAssignable<T, U> Observed<T>
        &operator-=(U const& rhs)
        {
            this->contained_ -= rhs;
            return *this;
        }

        template <typename T = ContainedT>
        requires std::equality_comparable<T> && Fundamental<T> Observed& operator=(T&& t)
        {
            return assignChecked(t);
        }

        template <typename T = ContainedT>
        requires std::equality_comparable<T> Observed& assignChecked(T&& other)
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

      private:
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
                , ref_{ref}
            {}
            operator T&()
            {
                return ref_;
            }
            T& operator*()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return ref_;
            }
            T* operator->()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return &ref_;
            }
            T& get()
            {
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
                return ref_;
            }
            T const& getReadonly()
            {
                return ref_;
            }
            void operator=(T& ref)
            {
                ref_ = ref;
                owner_->insertRangeChecked(pos_, pos_, RangeStateType::Modify);
            }

          protected:
            ObservedContainer<ContainerT>* owner_;
            std::size_t pos_;
            T& ref_;
        };

        template <typename WrappedIterator, typename ContainerT>
        class IteratorWrapper
        {
          public:
            using iterator_category = std::random_access_iterator_tag;
            using value_type = typename WrappedIterator::value_type;
            using difference_type = typename WrappedIterator::difference_type;
            using pointer = typename WrappedIterator::pointer;
            using reference = typename WrappedIterator::reference;

          public:
            IteratorWrapper(ObservedContainer<ContainerT>* owner, WrappedIterator it)
                : owner_{owner}
                , it_{std::move(it)}
            {}
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
            reference operator*()
            {
                return ReferenceWrapper<value_type, ContainerT>{owner_, it_ - owner_->container_.begin(), *it_};
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
    class ObservedContainer : public ObservedBase
    {
      public:
        friend class ContainerWrapUtility::ReferenceWrapper<typename ContainerT::value_type, ContainerT>;

        using value_type = typename ContainerT::value_type;
        using allocator_type = typename ContainerT::allocator_type;
        using size_type = typename ContainerT::size_type;
        using difference_type = typename ContainerT::difference_type;
        using reference = ContainerWrapUtility::ReferenceWrapper<typename ContainerT::value_type, ContainerT>;
        using const_reference = typename ContainerT::const_reference;
        using pointer = typename ContainerT::pointer;
        using const_pointer = typename ContainerT::const_pointer;

        // TODO: need to wrap these:
        using iterator = ContainerWrapUtility::IteratorWrapper<typename ContainerT::iterator, ContainerT>;
        using const_iterator = typename ContainerT::const_iterator;
        using reverse_iterator =
            ContainerWrapUtility::IteratorWrapper<typename ContainerT::reverse_iterator, ContainerT>;
        using const_reverse_iterator = typename ContainerT::const_reverse_iterator;

      public:
        ObservedContainer()
            : contained_{}
            , rangeContext_{0}
        {}
        ObservedContainer(const ObservedContainer&) = delete;
        ObservedContainer(ObservedContainer&&) = default;
        ObservedContainer& operator=(const ObservedContainer&) = delete;
        ObservedContainer& operator=(ObservedContainer&&) = default;
        ~ObservedContainer() = default;

        template <typename T = ContainerT>
        ObservedContainer(T&& t)
            : contained_{std::forward<T>(t)}
            , rangeContext_{static_cast<long>(contained_.size())}
        {}
        template <typename T = ContainerT>
        ObservedContainer& operator=(T&& t)
        {
            contained_ = std::forward<T>(t);
            rangeContext_.reset(contained_.size(), true);
            update();
            return *this;
        }
        void assign(size_type count, const value_type& value)
        {
            contained_.assign(count, value);
            rangeContext_.reset(contained_.size(), true);
            update();
        }
        template <typename Iterator>
        void assign(Iterator first, Iterator last)
        {
            contained_.assign(first, last);
            rangeContext_.reset(contained_.size(), true);
            update();
        }
        void assign(std::initializer_list<value_type> ilist)
        {
            contained_.assign(ilist);
            rangeContext_.reset(contained_.size(), true);
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
            return contained_.data();
        }
        const_pointer data() const noexcept
        {
            return contained_.data();
        }
        reference at(size_type pos)
        {
            reference ref = contained_.at(pos);
            return reference{this, pos, ref};
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
        // TODO:
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
        Detail::PickFirst_t<void, decltype(std::declval<U>().reserve())> reserve(size_type capacity)
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
            rangeContext_.reset(0, true);
            update();
        }
        iterator insert(const_iterator pos, const value_type& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, value);
            insertRangeChecked(distance, distance, RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator insert(const_iterator pos, value_type&& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, std::move(value));
            insertRangeChecked(distance, distance, RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator insert(const_iterator pos, size_type count, const value_type& value)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, count, value);
            insertRangeChecked(distance, distance + count, RangeStateType::Insert);
            return iterator{this, it};
        }
        template <typename Iterator>
        iterator insert(const_iterator pos, Iterator first, Iterator last)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, first, last);
            insertRangeChecked(distance, distance + std::distance(first, last), RangeStateType::Insert);
            return iterator{this, it};
        }
        iterator insert(const_iterator pos, std::initializer_list<value_type> ilist)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.insert(pos, ilist);
            insertRangeChecked(distance, distance + ilist.size(), RangeStateType::Insert);
            return iterator{this, it};
        }
        template <typename... Args>
        iterator emplace(const_iterator pos, Args&&... args)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.emplace(pos, std::forward<Args>(args)...);
            insertRangeChecked(distance, distance + sizeof...(Args), RangeStateType::Insert);
            return iterator{this, it};
        }
        // FIXME: does not work correctly
        iterator erase(iterator pos)
        {
            const auto distance = pos - begin();
            auto it = contained_.erase(pos.getWrapped());
            insertRangeChecked(distance, distance, RangeStateType::Erase);
            return iterator{this, it};
        }
        // FIXME: does not work correctly
        iterator erase(const_iterator pos)
        {
            const auto distance = pos - cbegin();
            auto it = contained_.erase(pos);
            insertRangeChecked(distance, distance, RangeStateType::Erase);
            return iterator{this, it};
        }
        // FIXME: does not work correctly
        iterator erase(iterator first, iterator last)
        {
            const auto distance = first - cbegin();
            auto it = contained_.erase(first.getWrapped(), last.getWrapped());
            insertRangeChecked(distance, distance + std::distance(first, last), RangeStateType::Erase);
            return iterator{this, it};
        }
        // FIXME: does not work correctly
        iterator erase(const_iterator first, const_iterator last)
        {
            const auto distance = first - cbegin();
            auto it = contained_.erase(first, last);
            insertRangeChecked(distance, distance + std::distance(first, last), RangeStateType::Erase);
            return iterator{this, it};
        }
        void push_back(const value_type& value)
        {
            contained_.push_back(value);
            insertRangeChecked(size() - 1, size() - 1, RangeStateType::Insert);
        }
        void push_back(value_type&& value)
        {
            contained_.push_back(std::move(value));
            insertRangeChecked(size() - 1, size() - 1, RangeStateType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_front())> push_front(const value_type& value)
        {
            contained_.push_front(value);
            insertRangeChecked(0, 0, RangeStateType::Insert);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().push_front())> push_front(value_type&& value)
        {
            contained_.push_front(std::move(value));
            insertRangeChecked(0, 0, RangeStateType::Insert);
        }
        template <typename... Args>
        void emplace_back(Args&&... args)
        {
            contained_.emplace_back(std::forward<Args>(args)...);
            insertRangeChecked(size() - 1, size() - 1, RangeStateType::Insert);
        }
        template <typename U = ContainerT, typename... Args>
        Detail::PickFirst_t<void, decltype(std::declval<U>().emplace_front())> emplace_front(Args&&... args)
        {
            contained_.emplace_front(std::forward<Args>(args)...);
            insertRangeChecked(0, 0, RangeStateType::Insert);
        }
        void pop_back()
        {
            contained_.pop_back();
            insertRangeChecked(size(), size(), RangeStateType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().pop_front())> pop_front()
        {
            contained_.pop_front();
            insertRangeChecked(0, 0, RangeStateType::Erase);
        }
        template <typename U = ContainerT>
        Detail::PickFirst_t<void, decltype(std::declval<U>().resize())> resize(size_type count)
        {
            const auto sizeBefore = contained_.size();
            contained_.resize(count);
            if (sizeBefore < count)
                insertRangeChecked(sizeBefore, count, RangeStateType::Insert);
            else
                insertRangeChecked(count, sizeBefore, RangeStateType::Erase);
        }
        void swap(ContainerT& other)
        {
            contained_.swap(other);
            rangeContext_.reset(contained_.size(), true);
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
            return rangeContext_;
        }
        RangeEventContext const& rangeContext() const
        {
            return rangeContext_;
        }

      private:
        void insertRangeChecked(std::size_t low, std::size_t high, RangeStateType type)
        {
            const auto result = rangeContext_.insertModificationRange(contained_.size(), low, high, type);
            if (result == RangeEventContext::InsertResult::Final)
            {
                update();
                globalEventContext.executeActiveEventsImmediately();
            }
            else if (result == RangeEventContext::InsertResult::Retry)
            {
                update();
                globalEventContext.executeActiveEventsImmediately();
                const auto innerResult = rangeContext_.insertModificationRange(contained_.size(), low, high, type);
                if (innerResult == RangeEventContext::InsertResult::Final)
                {
                    update();
                    globalEventContext.executeActiveEventsImmediately();
                }
                else if (innerResult == RangeEventContext::InsertResult::Retry)
                {
                    std::cout << "RangeEventContext::insertModificationRange() returned Retry twice in a row.\n";
                }
                else
                    update();
            }
            else
                update();
        }

      private:
        ContainerT contained_;
        mutable RangeEventContext rangeContext_;
    };

    template <typename... Parameters>
    class Observed<std::vector<Parameters...>> : public ObservedContainer<std::vector<Parameters...>>
    {};
    template <typename... Parameters>
    class Observed<std::deque<Parameters...>> : public ObservedContainer<std::deque<Parameters...>>
    {};

    template <typename T>
    requires Incrementable<T>
    inline Observed<T>& operator++(Observed<T>& observedValue)
    {
        ++observedValue.contained_;
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    requires Incrementable<T>
    inline T operator++(Observed<T>& observedValue, int)
    {
        auto tmp = observedValue.contained_;
        ++observedValue.contained_;
        observedValue.update();
        return tmp;
    }

    template <typename T>
    inline auto operator--(Observed<T>& observedValue)
        -> Observed<Detail::PickFirst_t<T, decltype(--std::declval<T>())>>&
    {
        --observedValue.contained_;
        observedValue.update();
        return observedValue;
    }
    template <typename T>
    inline auto operator--(Observed<T>& observedValue, int) -> Detail::PickFirst_t<T, decltype(std::declval<T>()--)>
    {
        auto tmp = observedValue.contained_;
        --observedValue.contained_;
        observedValue.update();
        return tmp;
    }

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
        constexpr bool IsObserved_v = IsObserved<T>::value;
    }
}