#pragma once

#include <nui/concepts.hpp>

#include <vector>
#include <utility>
#include <optional>
#include <algorithm>
#include <limits>
#include <set>
#include <iterator>

namespace Nui
{
    namespace Detail
    {
        struct InplaceTag
        {};
    }

    /**
     * This container associates items with an id and allows for individual items to be "selected",
     * which removes them from the container and stores them in a separate container. This allows
     * for efficient iteration over the selected items. Selected items can be reinserted into the
     * container via deselect (or not when the deselect callback returns false).
     *
     * @tparam T Type of the items to store.
     */
    template <typename T>
    class SelectablesRegistry
    {
      public:
        /// @brief Id type used to identify items.
        using IdType = std::size_t;

        /**
         * @brief Wrapper around items that associates them with an id.
         */
        struct ItemWithId
        {
            /// @brief Id of the item.
            IdType id;

            /**
             * @brief The item.
             *
             * The item is stored in an optional to allow for efficient "removal" of items from the core container.
             */
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
            ~ItemWithId() = default;

            /// @brief Compares the id of the item.
            bool operator<(ItemWithId const& other) const
            {
                return id < other.id;
            }
        };

        /// @brief Type of the container that stores the items.
        using ItemContainerType = std::vector<ItemWithId>;

        /// @brief Invalid id value.
        constexpr static auto invalidId = std::numeric_limits<IdType>::max();

        /**
         * @brief Iterator that ignores items that are selected.
         *
         * This iterator is also a safe iterator, performing range checking, which is required but also adds overhead.
         *
         * @tparam WrappedIterator The underlying iterator type.
         */
        template <typename WrappedIterator>
        class IteratorBase
        {
          public:
            IteratorBase(WrappedIterator wrapped, WrappedIterator begin, WrappedIterator end)
                : wrappedIterator_{std::move(wrapped)}
                , beginIterator_{std::move(begin)}
                , endIterator_{std::move(end)}
            {
                if (wrappedIterator_ == endIterator_)
                    return;
                // shift iterator to first valid item
                while (wrappedIterator_ != endIterator_ && !wrappedIterator_->item)
                    ++wrappedIterator_;
            }
            IteratorBase(IteratorBase const&) = default;
            IteratorBase(IteratorBase&&) = default;
            IteratorBase& operator=(IteratorBase const&) = default;
            IteratorBase& operator=(IteratorBase&&) = default;
            ~IteratorBase() = default;

            IteratorBase& operator++()
            {
                ++wrappedIterator_;
                while (wrappedIterator_ != endIterator_ && !wrappedIterator_->item)
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
                while (wrappedIterator_ != beginIterator_ && !wrappedIterator_->item)
                    --wrappedIterator_;
                if (wrappedIterator_ == beginIterator_ && !wrappedIterator_->item)
                    wrappedIterator_ = endIterator_;
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
                for (std::size_t i = 0; i < offset && tmp != endIterator_; ++i)
                    ++tmp;
                return tmp;
            }

            IteratorBase operator-(std::size_t offset) const
            {
                IteratorType tmp = *this;
                for (std::size_t i = 0; i < offset && tmp != beginIterator_; ++i)
                    --tmp;
                return tmp;
            }

            IteratorBase& operator+=(std::size_t offset)
            {
                for (std::size_t i = 0; i < offset && *this != endIterator_; ++i)
                    ++*this;
            }

            IteratorBase& operator-=(std::size_t offset)
            {
                for (std::size_t i = 0; i < offset && *this != beginIterator_; ++i)
                    --*this;
            }

            bool isEnd() const
            {
                return wrappedIterator_ == endIterator_;
            }

          protected:
            WrappedIterator wrappedIterator_;
            WrappedIterator beginIterator_;
            WrappedIterator endIterator_;
        };

        template <typename WrappedIterator>
        class ConstIterator : public IteratorBase<WrappedIterator>
        {
          public:
            using IteratorBase<WrappedIterator>::IteratorBase;
            using IteratorBase<WrappedIterator>::operator=;
            using IteratorBase<WrappedIterator>::wrappedIterator_;

            explicit ConstIterator(typename ItemContainerType::iterator iter)
                : IteratorBase<WrappedIterator>{std::move(iter)}
            {}

            auto const& operator*() const
            {
                if (IteratorBase<WrappedIterator>::isEnd())
                    throw std::runtime_error{"Dereferencing end iterator"};
                return wrappedIterator_->item.value();
            }

            auto const* operator->() const
            {
                if (IteratorBase<WrappedIterator>::isEnd())
                    throw std::runtime_error{"Dereferencing end iterator"};
                return &wrappedIterator_->item.value();
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
                if (IteratorBase<WrappedIterator>::isEnd())
                    throw std::runtime_error{"Dereferencing end iterator"};
                return wrappedIterator_->item.value();
            }

            auto* operator->()
            {
                if (IteratorBase<WrappedIterator>::isEnd())
                    throw std::runtime_error{"Dereferencing end iterator"};
                return &wrappedIterator_->item.value();
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

        /**
         * @brief Append an item to the container.
         *
         * @param element A new item to append.
         * @return IdType The id of the new item.
         */
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

        /**
         * @brief Append an item to the container.
         *
         * @param element A new item to append.
         * @return IdType The id of the new item.
         */
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

        /**
         * @brief Emplace an item to the container.
         *
         * @tparam Args Types of the arguments to forward to the constructor of the item.
         * @param args Arguments to forward to the constructor of the item.
         * @return IdType The id of the new item.
         */
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

        /**
         * @brief Erase/Remove an item from the container.
         *
         * @param id Id of the item to erase.
         * @return IteratorType Iterator to the next item.
         */
        IteratorType erase(IdType id)
        {
            const auto p = findItem(id);
            if (p == std::end(items_))
                return end();

            p->item.reset();
            --itemCount_;

            auto result = condense();
            return {result, items_.begin(), items_.end()};
        }

        /**
         * @brief Erase/Remove an item from the container and return it.
         *
         * @param id Id of the item to get and erase.
         * @return std::optional<T> The erased item.
         */
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
            /// @brief Pointer to the selected item (may be nullptr).
            std::optional<T>* item;

            /// @brief Whether the item was found.
            bool found;

            /// @brief Whether the item was already selected.
            bool alreadySelected;
        };

        /**
         * @brief Select an item.
         *
         * @param id The id of the item to select.
         * @return SelectionResult The result of the selection.
         */
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

        /**
         * @brief Deselects all items. Will reinsert items when callback returns true.
         *
         * @param callback callback to execute on each item.
         * @return std::size_t Amount of reinserted elements.
         */
        std::size_t deselectAll(std::invocable<ItemWithId const&> auto const& callback)
        {
            std::size_t result = 0;
            for (auto const& selected : selected_)
            {
                auto const id = selected.id;
                if (callback(selected))
                {
                    auto entry = findItem(id);
                    if (entry != std::end(items_))
                    {
                        ++itemCount_;
                        ++result;
                        // selected is a set item, and therefore immutable, so we need to const_cast to move it.
                        // this would break the set, but its cleared afterwards anyway.
                        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
                        entry->item = std::move(const_cast<ItemWithId&>(selected).item);
                    }
                }
            }
            selected_.clear();
            condense();
            return result;
        }

        /**
         * @brief Deselects item with id. Will reinsert item when callback returns true.
         *
         * @param id id of the item to deselect
         * @param callback callback to execute on the item
         * @return true Deselected and reinserted item
         * @return false Either did not find item or item was not reinserted
         */
        bool deselect(IdType id, std::invocable<ItemWithId const&> auto const& callback)
        {
            auto const iter = selected_.find(ItemWithId{id, std::nullopt});
            if (iter == std::end(selected_))
                return false;

            if (callback(*iter))
            {
                auto entry = findItem(id);
                if (entry != std::end(items_))
                {
                    ++itemCount_;
                    entry->item = std::move(const_cast<ItemWithId&>(*iter).item);
                }
                return true;
            }
            selected_.erase(iter);
            if (selected_.empty())
                condense();
            return false;
        }

        void clear()
        {
            items_.clear();
            selected_.clear();
            itemCount_ = 0;
            id_ = 0;
        }

        /**
         * @brief Get iterator to item with id.
         *
         * @param id Id of the item to get.
         * @return IteratorType Iterator to the item.
         */
        IteratorType get(IdType id)
        {
            auto iter = findItem(id);
            if (iter == std::end(items_))
                return end();
            return IteratorType{iter, items_.begin(), items_.end()};
        }

        /**
         * @brief Get iterator to item with id.
         *
         * @param id Id of the item to get.
         * @return ConstIteratorType Iterator to the item.
         */
        ConstIteratorType get(IdType id) const
        {
            auto iter = findItem(id);
            if (iter == std::end(items_))
                return end();
            return ConstIteratorType{iter, items_.begin(), items_.end()};
        }

        /**
         * @brief Returns item by id.
         *
         * @param id Id of the item to get.
         * @return auto const& Reference to the item.
         */
        auto const& operator[](IdType id) const
        {
            return *get(id);
        }

        /**
         * @brief Returns item by id.
         *
         * @param id Id of the item to get.
         * @return auto& Reference to the item.
         */
        auto& operator[](IdType id)
        {
            return *get(id);
        }

        /**
         * @brief Returns iterator to first unselected item or end.
         */
        IteratorType begin()
        {
            return {items_.begin(), items_.begin(), items_.end()};
        }

        /**
         * @brief Returns iterator to first unselected item or end.
         */
        ConstIteratorType begin() const
        {
            return {items_.begin(), items_.begin(), items_.end()};
        }

        /**
         * @brief Returns iterator to first unselected item or end.
         */
        ConstIteratorType cbegin() const
        {
            return {items_.cbegin(), items_.begin(), items_.end()};
        }

        /**
         * @brief Returns end iterator
         */
        IteratorType end()
        {
            return {items_.end(), items_.begin(), items_.end()};
        }

        /**
         * @brief Returns end iterator
         */
        ConstIteratorType end() const
        {
            return {items_.end(), items_.begin(), items_.end()};
        }

        /**
         * @brief Returns end iterator
         */
        ConstIteratorType cend() const
        {
            return {items_.cend(), items_.begin(), items_.end()};
        }

        /**
         * @brief Returns whether the container is empty.
         */
        bool empty() const
        {
            return itemCount_ == 0;
        }

        /**
         * @brief Returns the amount of items in the container.
         */
        std::size_t size() const
        {
            return itemCount_;
        }

        /**
         * @brief Returns an iterator to the underlying container.
         */
        typename ItemContainerType::iterator rawBegin()
        {
            return items_.begin();
        }

        /**
         * @brief Returns an iterator to the underlying container.
         */
        typename ItemContainerType::iterator rawEnd()
        {
            return items_.end();
        }

        /**
         * @brief Returns a const iterator to the underlying container.
         */
        typename ItemContainerType::const_iterator rawBegin() const
        {
            return items_.begin();
        }

        /**
         * @brief Returns a const iterator to the underlying container.
         */
        typename ItemContainerType::const_iterator rawConstBegin() const
        {
            return items_.cbegin();
        }

        /**
         * @brief Returns a const iterator to the underlying container.
         */
        typename ItemContainerType::const_iterator rawEnd() const
        {
            return items_.end();
        }

        /**
         * @brief Returns a const iterator to the underlying container.
         */
        typename ItemContainerType::const_iterator rawConstEnd() const
        {
            return items_.cend();
        }

        template <typename RegistryPtr>
        struct RawRangeWrap
        {
            RegistryPtr registry;
            typename ItemContainerType::iterator begin() const
            {
                return registry->rawBegin();
            }
            typename ItemContainerType::iterator end() const
            {
                return registry->rawEnd();
            }
            typename ItemContainerType::iterator cbegin() const
            {
                return registry->rawConstBegin();
            }
            typename ItemContainerType::iterator cend() const
            {
                return registry->rawConstEnd();
            }
        };

        /**
         * @brief Helper for range based for loops.
         *
         * @return RawRangeWrap
         */
        RawRangeWrap<SelectablesRegistry<T>*> rawRange()
        {
            return {this};
        }

        /**
         * @brief Helper for range based for loops.
         *
         * @return RawRangeWrap
         */
        RawRangeWrap<SelectablesRegistry<T> const*> rawRange() const
        {
            return {this};
        }

      private:
        typename ItemContainerType::iterator findItem(IdType id)
        {
            const auto p =
                std::lower_bound(std::begin(items_), std::end(items_), id, [](auto const& lhs, auto const& rhs) {
                    return lhs.id < rhs;
                });

            if (p == std::end(items_) || p->id != id)
                return std::end(items_);
            return p;
        }
        typename ItemContainerType::const_iterator findItem(IdType id) const
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
        ItemContainerType items_{};
        // TODO: improve performance, id link backs are costly, each one is a binary search.
        std::set<ItemWithId> selected_{};
        IdType itemCount_{0};
        IdType id_{0};
    };
}