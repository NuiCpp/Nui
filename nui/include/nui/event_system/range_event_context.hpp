#pragma once

#include <interval-tree/interval_tree.hpp>

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

        // template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        // std::vector<RangeStateInterval<ValueType, IntervalKind>> cutIntervals(
        //     RangeStateInterval<ValueType, IntervalKind> const& k,
        //     RangeStateInterval<ValueType, IntervalKind> const& m);

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
            void high(value_type value)
            {
                high_ = value;
            }
            bool overlaps(value_type l, value_type h) const
            {
                return low_ <= h && l <= high_;
            }
            bool overlaps_exclusive(value_type l, value_type h) const
            {
                return low_ < h && l < high_;
            }
            bool overlaps(RangeStateInterval const& other) const
            {
                return overlaps(other.low_, other.high_);
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
            std::vector<RangeStateInterval> join(RangeStateInterval const& other) const
            {
                return {RangeStateInterval{std::min(low_, other.low_), std::max(high_, other.high_)}};

                // auto typeWithoutExtension = static_cast<RangeOperationType>(other.type_ &
                // ~RangeOperationType::Extended); long extensionFix = other.type_ & RangeOperationType::Extended ? 1 :
                // 0; switch (typeWithoutExtension | type_)
                // {
                //     case (RangeOperationType::Modify | RangeOperationType::Modify):
                //     {
                //         return {
                //             {std::min(low_, other.low_ + extensionFix),
                //              std::max(high_, other.high_ - extensionFix),
                //              RangeOperationType::Modify}};
                //     }
                //     case (RangeOperationType::Keep | RangeOperationType::Modify):
                //     {
                //         // Modifications cut out of the keep part.
                //         return cutIntervals(
                //             *this, {other.low_ + extensionFix, other.high_ - extensionFix, typeWithoutExtension});
                //     }
                //     default:
                //     {
                //         throw std::runtime_error("Invalid insertion case");
                //     }
                // }
            }

          private:
            value_type low_;
            value_type high_;
        };

        // stream iterator for intervals
        template <typename T, typename Kind>
        std::ostream& operator<<(std::ostream& os, RangeStateInterval<T, Kind> const& interval)
        {
            os << "[" << interval.low() << ", " << interval.high() << "]";
            return os;
        }

        // template <typename ValueType, typename IntervalKind>
        // std::vector<RangeStateInterval<ValueType, IntervalKind>> cutIntervals(
        //     RangeStateInterval<ValueType, IntervalKind> const& k,
        //     RangeStateInterval<ValueType, IntervalKind> const& m)
        // {
        //     std::vector<RangeStateInterval<ValueType, IntervalKind>> result;
        //     if (k.low() <= m.low() - 1)
        //         result.emplace_back(k.low(), m.low() - 1, RangeOperationType::Keep);
        //     result.emplace_back(m.low(), m.high(), RangeOperationType::Modify);
        //     if (k.high() >= m.high() + 1)
        //         result.emplace_back(m.high() + 1, k.high(), RangeOperationType::Keep);
        //     return result;
        // }
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

            auto newRange = Detail::RangeStateInterval<long>{low, high};

            switch (type)
            {
                default:
                    return InsertResult::Rejected;
                case RangeOperationType::Modify:
                {
                    // Insert and merge interval only:
                    trackedRanges_.insert_overlap(newRange);
                    return InsertResult::Accepted;
                }
                case RangeOperationType::Insert:
                {
                    // TODO:
                }
                case RangeOperationType::Erase:
                {
                    auto& sourceSpace = trackedRanges_;
                    bool processed = false;
                    for (auto it = sourceSpace.begin(); it != sourceSpace.end(); ++it)
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
                                it = sourceSpace.erase(it);

                                // Comment back in if erase is ever found to be broken :D
                                // it = sourceSpace.overlap_find(newRange, false);
                            } while (it != sourceSpace.end() && it->overlapsOrIsAdjacent(newRange));
                            sourceSpace.insert(newRange);
                            processed = true;
                            break;
                        }
                        else
                        {
                            break;
                        }
                    }
                    if (!processed)
                    {
                        sourceSpace.insert_overlap(newRange);
                    }
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
        // lib_interval_tree::interval_tree<Detail::RangeStateInterval<long>> eraseSourceSpace_;
        lib_interval_tree::interval_tree<Detail::RangeStateInterval<long>> trackedRanges_;
        RangeOperationType operationType_;
        long dataSize_;
        bool fullRangeUpdate_;
        bool disableOptimizations_;
    };
}