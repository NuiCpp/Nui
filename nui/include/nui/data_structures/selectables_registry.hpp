#pragma once

#include <nui/concepts.hpp>

#include <vector>
#include <utility>
#include <optional>
#include <algorithm>
#include <variant>
#include <limits>
#include <set>

#include <iostream>

namespace Nui
{
    namespace Detail
    {
        struct InplaceTag
        {};
    }

    template <typename T>
    class SelectablesRegistry
    {
      public:
        using IdType = std::size_t;
        struct ItemWithId
        {
            IdType id;
            std::optional<T> item;

            template <typename... Args>
            ItemWithId(IdType id, Detail::InplaceTag, Args&&... args)
                : id{id}
                , item{T{std::forward<Args>(args)...}}
            {}
            ItemWithId(IdType id, T item)
                : id{id}
                , item{std::move(item)}
            {}
            ItemWithId(IdType id, std::optional<T> item)
                : id{id}
                , item{std::move(item)}
            {}
            ItemWithId(ItemWithId const&) = default;
            ItemWithId(ItemWithId&&) = default;
            ItemWithId& operator=(ItemWithId const&) = default;
            ItemWithId& operator=(ItemWithId&&) = default;

            bool operator<(ItemWithId const& other) const
            {
                return id < other.id;
            }
        };
        using ItemContainerType = std::vector<ItemWithId>;
        constexpr static auto invalidId = std::numeric_limits<IdType>::max();

        template <typename WrappedIterator>
        class IteratorBase
        {
          public:
            IteratorBase(WrappedIterator wrapped)
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

            friend bool operator==(const IteratorBase& lhs, const IteratorBase& rhs)
            {
                return lhs.wrappedIterator_ == rhs.wrappedIterator_;
            }

            friend bool operator!=(const IteratorBase& lhs, const IteratorBase& rhs)
            {
                return !(lhs == rhs);
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
            using IteratorBase<WrappedIterator>::wrappedIterator_;

            ConstIterator(typename ItemContainerType::iterator iter)
                : IteratorBase<WrappedIterator>{std::move(iter)}
            {}

            auto const& operator*() const
            {
                return *wrappedIterator_;
            }

            auto const* operator->() const
            {
                return &*wrappedIterator_;
            }
        };

        template <typename WrappedIterator>
        class Iterator : public IteratorBase<WrappedIterator>
        {
          public:
            using IteratorBase<WrappedIterator>::IteratorBase;
            using IteratorBase<WrappedIterator>::operator=;
            using IteratorBase<WrappedIterator>::wrappedIterator_;

            auto& operator*()
            {
                return *wrappedIterator_;
            }

            auto* operator->()
            {
                return &*wrappedIterator_;
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

        IdType append(T const& element)
        {
            items_.push_back(ItemWithId{id_, std::optional<T>{element}});
            ++itemCount_;
            const auto id = id_;
            id_++;
            if (id_ == invalidId - 1)
                id_ = 0;
            return id;
        }
        IdType append(T&& element)
        {
            items_.push_back(ItemWithId{id_, std::optional<T>{std::move(element)}});
            ++itemCount_;
            const auto id = id_;
            id_++;
            if (id_ == invalidId - 1)
                id_ = 0;
            return id;
        }

        template <typename... Args>
        IdType emplace(Args&&... args)
        {
            items_.push_back(ItemWithId{id_, Detail::InplaceTag{}, std::forward<Args>(args)...});
            ++itemCount_;
            const auto id = id_;
            id_++;
            if (id_ == invalidId - 1)
                id_ = 0;
            return id;
        }

        IteratorType erase(IdType id)
        {
            const auto p = findItem(id);
            if (p == std::end(items_))
                return end();

            p->item.reset();
            --itemCount_;

            auto result = condense();
            if (result != std::end(items_))
                return result;

            return p;
        }

        std::optional<T> pop(IdType id)
        {
            const auto p = findItem(id);
            if (p == std::end(items_))
                return std::nullopt;

            const auto result = std::move(p->item);
            p->item.reset();
            --itemCount_;

            condense();
            return result;
        }

        struct SelectionResult
        {
            std::optional<T>* item;
            bool found;
            bool alreadySelected;
        };
        SelectionResult select(IdType id)
        {
            const auto iter = findItem(id);
            if (iter == std::end(items_))
                return {nullptr, false, false};
            if (!iter->item.has_value())
                return {nullptr, true, true};

            --itemCount_;

            const auto selectedIter = selected_.insert(std::move(*iter)).first;
            iter->item.reset();
            // having modifying access to the optional<T> does not mess with the set ordering. const casting is fine
            // here.
            return {
                .item = &(const_cast<ItemWithId&>(*selectedIter).item),
                .found = true,
                .alreadySelected = false,
            };
        }

        void deselectAll(std::invocable<ItemWithId const&> auto callback)
        {
            for (auto const& selected : selected_)
            {
                auto const id = selected.id;
                if (callback(selected))
                {
                    ++itemCount_;
                    auto entry = get(id);
                    if (entry != end())
                        entry->item = std::move(const_cast<ItemWithId&>(selected).item);
                }
            }
            selected_.clear();
            condense();
        }

        void deselect(IdType id, std::invocable<ItemWithId const&> auto callback)
        {
            auto const iter = selected_.find(ItemWithId{id, std::nullopt});
            if (iter == std::end(selected_))
                return;

            if (callback(*iter))
            {
                ++itemCount_;
                auto entry = get(id);
                if (entry != end())
                    entry->item = std::move(const_cast<ItemWithId&>(*iter).item);
            }
            selected_.erase(iter);
            if (selected_.empty())
                condense();
        }

        IteratorType get(IdType id)
        {
            auto iter = findItem(id);
            if (iter == std::end(items_))
                return end();
            return IteratorType{iter};
        }

        ConstIteratorType get(IdType id) const
        {
            auto iter = findItem(id);
            if (iter == std::end(items_))
                return end();
            return ConstIteratorType{iter};
        }

        auto const& operator[](IdType id) const
        {
            return *get(id);
        }
        auto& operator[](IdType id)
        {
            return *get(id);
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

        auto condense()
        {
            if (selected_.empty() && itemCount_ < (items_.size() / 2))
            {
                return items_.erase(
                    std::remove_if(
                        std::begin(items_),
                        std::end(items_),
                        [](auto const& item) {
                            return !item.item;
                        }),
                    std::end(items_));
            }
            return std::end(items_);
        }

      private:
        std::vector<ItemWithId> items_;
        // TODO: improve performance, id link backs are costly, each one is a binary search.
        std::set<ItemWithId> selected_;
        IdType itemCount_;
        IdType id_;
    };
}