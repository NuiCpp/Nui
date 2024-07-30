#pragma once

#include <nui/utility/assert.hpp>

#include <interval-tree/interval_tree.hpp>
#include <interval-tree/tree_hooks.hpp>

#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <optional>

namespace Nui
{
    enum RangeOperationType
    {
        Keep = 0b0001,
        Modify = 0b0010,
        Insert = 0b0100,
        Erase = 0b1000
    };

    namespace Detail
    {
        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        class RangeStateInterval;

        template <typename ValueType, typename IntervalKind>
        class RangeStateInterval
        {
          public:
            using value_type = ValueType;
            using interval_kind = IntervalKind;

          public:
            RangeStateInterval(value_type low, value_type high)
                : low_{low}
                , high_{high}
            {}
            void reset(value_type low, value_type high)
            {
                low_ = low;
                high_ = high;
            }
            friend bool operator==(RangeStateInterval const& lhs, RangeStateInterval const& rhs)
            {
                return lhs.start_ == rhs.start_ && lhs.end_ == rhs.end_ && lhs.type_ == rhs.type_;
            }
            friend bool operator!=(RangeStateInterval const& lhs, RangeStateInterval const& rhs)
            {
                return !(lhs == rhs);
            }
            value_type low() const
            {
                return low_;
            }
            value_type high() const
            {
                return high_;
            }
            void low(value_type value)
            {
                low_ = value;
            }
            void high(value_type value)
            {
                high_ = value;
            }
            bool overlaps(value_type l, value_type h) const
            {
                // return low_ <= h && l <= high_;
                return overlapsOrIsAdjacent(l, h);
            }
            // looks inclusive, but inclusive now means adjacent:
            bool overlaps_exclusive(value_type l, value_type h) const
            {
                return low_ <= h && l <= high_;
            }
            bool overlaps(RangeStateInterval const& other) const
            {
                // return overlaps(other.low_, other.high_);
                return overlapsOrIsAdjacent(other);
            }
            bool overlapsOrIsAdjacent(value_type l, value_type h) const
            {
                return low_ <= h + 1 && l - 1 <= high_;
            }
            bool overlapsOrIsAdjacent(RangeStateInterval const& other) const
            {
                return low_ <= other.high_ + 1 && other.low_ - 1 <= high_;
            }
            bool overlaps_exclusive(RangeStateInterval const& other) const
            {
                return overlaps_exclusive(other.low_, other.high_);
            }
            bool within(value_type value) const
            {
                return interval_kind::within(low_, high_, value);
            }
            bool within(RangeStateInterval const& other) const
            {
                return low_ <= other.low_ && high_ >= other.high_;
            }
            void shiftRight(value_type offset)
            {
                low_ += offset;
                high_ += offset;
            }
            void shiftLeft(value_type offset)
            {
                low_ -= offset;
                high_ -= offset;
            }
            bool isLeftOf(RangeStateInterval const& other) const
            {
                return high_ < other.low_;
            }
            value_type operator-(RangeStateInterval const& other) const
            {
                if (overlaps(other))
                    return 0;
                if (high_ < other.low_)
                    return other.low_ - high_;
                else
                    return low_ - other.high_;
            }
            value_type size() const
            {
                return high_ - low_;
            }
            // undefined if they do not overlap
            RangeStateInterval expand(RangeStateInterval const& other) const
            {
                const auto low = std::min(low_, other.low_);
                // +1, because if they overlap they share a side, so its not double counted
                // [0, 1] and [1, 2] -> [0, 2]
                // [8, 8] and [8, 8] -> [8, 9]
                const auto high = low + size() + other.size() + 1;
                return {low, high};
            }
            RangeStateInterval join(RangeStateInterval const& other) const
            {
                return RangeStateInterval{std::min(low_, other.low_), std::max(high_, other.high_)};
            }

          private:
            value_type low_;
            value_type high_;
        };

        struct CustomIntervalTreeNode
            : public lib_interval_tree::node<long, RangeStateInterval<long>, CustomIntervalTreeNode>
        {
            using lib_interval_tree::node<long, RangeStateInterval<long>, CustomIntervalTreeNode>::node;

            void shiftRight(long offset)
            {
                this->interval_.low(this->interval_.low() + offset);
                this->interval_.high(this->interval_.high() + offset);
                this->max_ = this->max_ + offset;
            }

            void shiftLeft(long offset)
            {
                this->interval_.low(this->interval_.low() - offset);
                this->interval_.high(this->interval_.high() - offset);
                this->max_ = this->max_ - offset;
            }
        };

        struct IntervalTreeHook : public lib_interval_tree::hooks::regular
        {
            using node_type = CustomIntervalTreeNode;
        };

        // stream iterator for intervals
        template <typename T, typename Kind>
        std::ostream& operator<<(std::ostream& os, RangeStateInterval<T, Kind> const& interval)
        {
            os << "[" << interval.low() << ", " << interval.high() << "]";
            return os;
        }
    }

    class RangeEventContext
    {
      public:
        explicit RangeEventContext(long dataSize)
            : RangeEventContext(dataSize, false)
        {}
        RangeEventContext(long dataSize, bool disableOptimizations)
            : trackedRanges_{}
            , operationType_{RangeOperationType::Keep}
            , dataSize_{dataSize}
            , nextEraseOverride_{std::nullopt}
            , fullRangeUpdate_{true}
            , disableOptimizations_{disableOptimizations}
        {
            reset(dataSize, true);
        }
        enum class InsertResult
        {
            Perform,
            PerformAndRetry,
            Accepted,
            Rejected
        };
        void performFullRangeUpdate()
        {
            fullRangeUpdate_ = true;
        }
        /// @brief Done before the erasure is performed:
        /// @return true if a fixup was performed
        bool eraseNotify(long low, long high)
        {
            if (operationType_ == RangeOperationType::Keep || operationType_ == RangeOperationType::Erase ||
                fullRangeUpdate_ || disableOptimizations_ || trackedRanges_.empty())
                return false;
            if (operationType_ == RangeOperationType::Modify)
                return eraseModificationFixup(low, high);
            else
                return eraseInsertionFixup(low, high);
        }
        bool eraseNotify(std::size_t low, std::size_t high)
        {
            return eraseNotify(static_cast<long>(low), static_cast<long>(high));
        }
        InsertResult insertModificationRange(long elementCount, long low, long high, RangeOperationType type)
        {
            if (disableOptimizations_)
            {
                fullRangeUpdate_ = true;
                return InsertResult::Perform;
            }

            if (operationType_ == RangeOperationType::Keep)
            {
                operationType_ = type;
            }
            else if (operationType_ != type)
                return InsertResult::PerformAndRetry;

            switch (type)
            {
                default:
                    return InsertResult::Rejected;
                case RangeOperationType::Modify:
                {
                    // Insert and merge interval only:
                    trackedRanges_.insert_overlap({low, high});
                    break;
                }
                case RangeOperationType::Insert:
                {
                    insertInsertRange(elementCount, low, high);
                    break;
                }
                case RangeOperationType::Erase:
                {
                    if (nextEraseOverride_)
                    {
                        insertEraseRange(elementCount, nextEraseOverride_->low(), nextEraseOverride_->high());
                        nextEraseOverride_ = std::nullopt;
                    }
                    else
                        insertEraseRange(elementCount, low, high);
                    break;
                }
            }

            return InsertResult::Accepted;
        }
        InsertResult
        insertModificationRange(std::size_t elementCount, std::size_t low, std::size_t high, RangeOperationType type)
        {
            return insertModificationRange(
                static_cast<long>(elementCount), static_cast<long>(low), static_cast<long>(high), type);
        }
        void reset(std::size_t dataSize)
        {
            reset(static_cast<long>(dataSize), false);
        }
        void reset(long dataSize, bool requireFullRangeUpdate)
        {
            trackedRanges_.clear();
            dataSize_ = dataSize;
            fullRangeUpdate_ = requireFullRangeUpdate;
            operationType_ = RangeOperationType::Keep;
        }
        bool isFullRangeUpdate() const noexcept
        {
            return fullRangeUpdate_;
        }
        auto begin() const
        {
            return trackedRanges_.begin();
        }
        auto end() const
        {
            return trackedRanges_.end();
        }
        auto rbegin() const
        {
            return trackedRanges_.rbegin();
        }
        auto rend() const
        {
            return trackedRanges_.rend();
        }
        RangeOperationType operationType() const noexcept
        {
            return operationType_;
        }

      private:
        void insertInsertRange(long elementCount, long low, long high)
        {
            // TODO: perform optimization using interval tree specialization for shifting subtrees.
            auto newRange = Detail::RangeStateInterval<long>{low, high};

            // find insert at same position first to move previous insert at that position over:
            auto it = trackedRanges_.find(newRange, [](auto const& a, auto const& b) {
                return a.low() == b.low();
            });

            if (it != trackedRanges_.end())
            {
                newRange = it->expand(newRange);
                trackedRanges_.erase(it);
                it = trackedRanges_.insert(newRange);
            }
            else
            {
                it = trackedRanges_.insert(newRange);
            }
            if (it != std::end(trackedRanges_))
                ++it;

            // move all subsequent intervals to the right:
            // TODO: This is the expensive part and should be optimized.
            for (; it != trackedRanges_.end(); ++it)
            {
                it.node()->shiftRight(high - low + 1);
            }
        }
        void insertEraseRange(long elementCount, long low, long high)
        {
            auto newRange = Detail::RangeStateInterval<long>{low, high};
            const bool processed = [&newRange, low, high, this]() {
                for (auto it = trackedRanges_.begin(); it != trackedRanges_.end(); ++it)
                {
                    if (it->low() < newRange.low())
                    {
                        // +1 because the range includes its end
                        newRange.shiftRight(it->size() + 1);
                    }
                    else if (it->overlapsOrIsAdjacent(newRange))
                    {
                        do
                        {
                            newRange = it->expand(newRange);
                            // This should not happen:
                            newRange.high(std::min(newRange.high(), dataSize_));

                            // Remove the old range and insert the new one
                            it = trackedRanges_.erase(it);

                            // Comment back in if erase is ever found to be broken :D
                            // it = trackedRanges_.overlap_find(newRange, false);
                        } while (it != trackedRanges_.end() && it->overlapsOrIsAdjacent(newRange));
                        trackedRanges_.insert(newRange);
                        return true;
                    }
                    else
                    {
                        return false;
                    }
                }
                return false;
            }();
            if (!processed)
            {
                trackedRanges_.insert_overlap(newRange);
            }
        }

        /**
         * @brief This is necessary to remove previous inserts that are now erased.
         *
         * @param low
         * @param high
         */
        bool eraseInsertionFixup(long low, long high)
        {
            const auto lastInterval = *trackedRanges_.rbegin();

            // If the erase interval is left of the last insert interval, we must apply the changes and
            // retry. The following optimization would make the insert positions invalid.
            // TODO: Moving them might be possible, but expensive.
            if (high < lastInterval.high())
                return true;

            auto eraseRange = Detail::RangeStateInterval<long>{low, high};
            auto eraseRangeOrig = eraseRange;

            auto iter = trackedRanges_.overlap_find(eraseRange, true);
            // if erase is completely at the end of a previous insert, we can cut the inserted elements out.
            for (; iter != trackedRanges_.end(); iter = trackedRanges_.overlap_find(eraseRangeOrig, true))
            {
                if (iter == trackedRanges_.end())
                    return true;
                const auto insertInterval = *iter;

                // erase overlaps insert to the end or over:
                if (eraseRangeOrig.high() >= insertInterval.high())
                {
                    trackedRanges_.erase(iter);
                    // if beginning of insert is left of erase, we have to insert the left part of the insert
                    if (insertInterval.low() < eraseRangeOrig.low())
                    {
                        trackedRanges_.insert({insertInterval.low(), eraseRangeOrig.low() - 1});
                        eraseRangeOrig.high(-insertInterval.high() + eraseRangeOrig.low() + eraseRangeOrig.high() - 1);
                    }

                    eraseRange.high(eraseRange.high() - insertInterval.size() - 1);
                }
                else
                {
                    break; // bail out
                }
            }

            if (eraseRange.high() != high)
                nextEraseOverride_ = eraseRange;

            // Other tracking cases are too complicated.
            // The reason is that cutting the intervals is not enough, we also have to modify where these insertions are
            // taken from.
            return true;
        }

        /**
         * @brief This is necessary to remove previous modifies that are now erased.
         *
         * @param low
         * @param high
         */
        bool eraseModificationFixup(long low, long high)
        {
            auto range = Detail::RangeStateInterval<long>{low, high};

            auto iter = trackedRanges_.overlap_find(range, false);

            if (iter == trackedRanges_.end())
            {
                // If we dont have an overlap, there might still be a case where modifications exists
                // after the erase. In that case we have to apply these changes immediately and retry the erase.
                // (Shifting the previous modifications would work too, but is more expensive)
                const auto lastInterval = *trackedRanges_.rbegin();

                // If the erase interval is left of the last modification interval, we must apply the changes and
                // retry. An overlap would have been found otherwise.
                if (high < lastInterval.low())
                    return true;

                return false;
            }

            // find all overlapping modifications and cut them
            for (; iter != trackedRanges_.end(); iter = trackedRanges_.overlap_find(range, false))
            {
                const auto modInterval = *iter;
                iter = trackedRanges_.erase(iter);

                // cut erasure from found modification interval:
                // 1. erase is within modification interval:
                if (modInterval.within(range))
                {
                    if (modInterval.low() < range.low())
                        trackedRanges_.insert({modInterval.low(), range.low() - 1});
                    if (range.high() < modInterval.high())
                        trackedRanges_.insert({range.high() + 1, modInterval.high()});
                    return true; // cannot overlap any further
                }
                else if (modInterval.low() < range.low())
                {
                    // 2. erase starts within modification interval:
                    trackedRanges_.insert({modInterval.low(), range.low() - 1});

                    // reduce range to right part:
                    range.low(range.low() + 1);
                    if (range.low() > range.high())
                        return true;
                }
                else if (range.high() < modInterval.high())
                {
                    // 3. erase ends within modification interval:
                    trackedRanges_.insert({range.high() + 1, modInterval.high()});

                    // reduce range to left part:
                    range.high(range.high() - 1);
                    if (range.low() > range.high())
                        return true;
                }
                else if (range.within(modInterval))
                {
                    // 4. erase encompasses modification interval, only deletion is necessary:
                    continue;
                }
                else
                {
                    NUI_ASSERT(false, "Overlap without overlapping?");
                }
            }
            return true;
        }

      private:
        lib_interval_tree::interval_tree<Detail::RangeStateInterval<long>, Detail::IntervalTreeHook> trackedRanges_;
        RangeOperationType operationType_;
        std::optional<Detail::RangeStateInterval<long>> nextEraseOverride_;
        long dataSize_;
        bool fullRangeUpdate_;
        bool disableOptimizations_;
    };
}