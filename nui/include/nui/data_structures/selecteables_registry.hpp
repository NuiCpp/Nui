#pragma once

#include <nui/concepts.hpp>

#include <vector>
#include <utility>
#include <optional>
#include <algorithm>
#include <variant>
#include <limits>

namespace Nui
{
    template <typename T>
    class SelectablesRegistry
    {
      public:
        using IdType = std::size_t;
        struct ItemWithId
        {
            IdType id;
            std::optional<T> item;
        };
        using ItemContainerType = std::vector<ItemWithId>;
        constexpr static auto invalidId = std::numeric_limits<IdType>::max();

        template <typename WrappedIterator>
        class IteratorBase
        {
          public:
            IteratorBase(WrappedIterator&& wrapped)
                : wrappedIterator_{std::move(wrapped)}
            {}
            IteratorBase(IteratorBase const&) = default;
            IteratorBase(IteratorBase&&) = default;
            IteratorBase& operator=(IteratorBase const&) = default;
            IteratorBase& operator=(IteratorBase&&) = default;
            ~IteratorBase() = default;

            IteratorBase& operator++()
            {
                ++wrappedIterator_;
                while (wrappedIterator_ != wrappedIterator_.end() && !wrappedIterator_->item)
                    ++wrappedIterator_;
                return *this;
            }

            IteratorBase operator++(int)
            {
                IteratorBase tmp = *this;
                ++*this;
                return tmp;
            }

            IteratorBase& operator--()
            {
                --wrappedIterator_;
                while (wrappedIterator_ != wrappedIterator_.begin() && !wrappedIterator_->item)
                    --wrappedIterator_;
                return *this;
            }

            IteratorBase operator--(int)
            {
                IteratorBase tmp = *this;
                --*this;
                return tmp;
            }

            bool operator==(const IteratorBase& rhs) const
            {
                return wrappedIterator_ == rhs.wrappedIterator_;
            }

            bool operator!=(const IteratorBase& rhs) const
            {
                return !(*this == rhs);
            }

            IteratorBase operator+(std::size_t offset) const
            {
                IteratorType tmp = *this;
                for (std::size_t i = 0; i < offset && tmp != wrappedIterator_.end(); ++i)
                    ++tmp;
                return tmp;
            }

            IteratorBase operator-(std::size_t offset) const
            {
                IteratorType tmp = *this;
                for (std::size_t i = 0; i < offset && tmp != wrappedIterator_.begin(); ++i)
                    --tmp;
                return tmp;
            }

            IteratorBase& operator+=(std::size_t offset)
            {
                for (std::size_t i = 0; i < offset && *this != wrappedIterator_.end(); ++i)
                    ++*this;
            }

            IteratorBase& operator-=(std::size_t offset)
            {
                for (std::size_t i = 0; i < offset && *this != wrappedIterator_.begin(); ++i)
                    --*this;
            }

          protected:
            WrappedIterator wrappedIterator_;
        };

        template <typename WrappedIterator>
        class ConstIterator : public IteratorBase<WrappedIterator>
        {
          public:
            using IteratorBase<WrappedIterator>::IteratorBase;
            using IteratorBase<WrappedIterator>::operator=;
            using IteratorBase<WrappedIterator>::operator==;
            using IteratorBase<WrappedIterator>::operator!=;
            using IteratorBase<WrappedIterator>::wrappedIterator_;

            T const& operator*() const
            {
                return *wrappedIterator_->item;
            }

            T const* operator->() const
            {
                return &*wrappedIterator_->item;
            }
        };

        template <typename WrappedIterator>
        class Iterator : public IteratorBase<WrappedIterator>
        {
          public:
            using IteratorBase<WrappedIterator>::IteratorBase;
            using IteratorBase<WrappedIterator>::operator=;
            using IteratorBase<WrappedIterator>::operator==;
            using IteratorBase<WrappedIterator>::operator!=;
            using IteratorBase<WrappedIterator>::wrappedIterator_;

            T& operator*()
            {
                return *wrappedIterator_->item;
            }

            T* operator->()
            {
                return &*wrappedIterator_->item;
            }
        };

        using IteratorType = Iterator<typename ItemContainerType::iterator>;
        using ConstIteratorType = ConstIterator<typename ItemContainerType::const_iterator>;

      public:
        SelectablesRegistry() = default;
        SelectablesRegistry(const SelectablesRegistry&) = default;
        SelectablesRegistry(SelectablesRegistry&&) = default;
        SelectablesRegistry& operator=(const SelectablesRegistry&) = default;
        SelectablesRegistry& operator=(SelectablesRegistry&&) = default;
        ~SelectablesRegistry() = default;

        IdType append(T element)
        {
            bool rewireSelected = !selected_.empty() && items_.capacity() == 0;

            items_.push_back(ItemWithId{id_, std::optional<T>{std::move(element)}});
            ++itemCount_;
            const auto id = id_;
            id_++;
            if (id_ == invalidId - 1)
                id_ = 0;
            if (rewireSelected)
            {
                for (auto& selected : selected_)
                    selected.second = findItem(selected.first.id);
            }
            return id;
        }

        IteratorType erase(IdType id)
        {
            const auto p = findItem(id);
            if (p == std::end(items_))
                return end();

            p->item.reset();
            --itemCount_;

            if (selected_.empty() && itemCount_ < (items_.size() / 2))
            {
                return items_.erase(
                    std::remove_if(
                        begin(items_),
                        end(items_),
                        [](auto const& item) {
                            return !item.item;
                        }),
                    end(items_));
            }
            return p;
        }

        bool select(IdType id)
        {
            const auto iter = findItem(id);
            if (iter == std::end(items_))
                return false;

            selected_.push_back(
                std::pair<ItemWithId, typename std::vector<ItemWithId>::iterator>{std::move(*iter), iter});
            iter->item.reset();
            return true;
        }

        void deselectAll(std::invocable<ItemWithId const&> auto callback)
        {
            for (auto& selected : selected_)
            {
                if (callback(selected.first))
                    *selected.second = std::move(selected.first);
            }
            selected_.clear();
        }

        IteratorType get(IdType id)
        {
            const auto iter = findItem(id);
            if (iter == std::end(items_))
                return end();
            return iter;
        }

        IteratorType get(IdType id) const
        {
            const auto p = std::lower_bound(begin(items_), end(items_), id, [](auto const& lhs, auto const& rhs) {
                return lhs.id < rhs.id;
            });

            if (p == std::end(items_) || p->id != id)
                return end();

            return p;
        }

        IteratorType operator[](IdType id) const
        {
            return get(id);
        }
        IteratorType operator[](IdType id)
        {
            return get(id);
        }

        IteratorType begin()
        {
            return {items_.begin()};
        }
        IteratorType begin() const
        {
            return {items_.begin()};
        }
        ConstIteratorType cbegin() const
        {
            return {items_.cbegin()};
        }
        IteratorType end()
        {
            return {items_.end()};
        }
        ConstIteratorType end() const
        {
            return {items_.end()};
        }
        ConstIteratorType cend() const
        {
            return {items_.cend()};
        }

      private:
        typename std::vector<ItemWithId>::iterator findItem(IdType id)
        {
            const auto p =
                std::lower_bound(std::begin(items_), std::end(items_), id, [](auto const& lhs, auto const& rhs) {
                    return lhs.id < rhs;
                });

            if (p == std::end(items_) || p->id != id)
                return std::end(items_);
            return p;
        }

      private:
        std::vector<ItemWithId> items_;
        std::vector<std::pair<ItemWithId, typename std::vector<ItemWithId>::iterator>> selected_;
        // unsigned for well defined overflow:
        std::size_t itemCount_;
        IdType id_;
    };
}