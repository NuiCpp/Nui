#pragma once

#include <interval-tree/interval_tree.hpp>

#include <cstddef>
#include <algorithm>
#include <stdexcept>
#include <vector>
#include <optional>

#include <iostream>
#include <iomanip>

namespace Nui
{
    enum RangeStateType
    {
        Keep = 0b0001,
        Modify = 0b0010,
        Insert = 0b0100,
        Erase = 0b1000,
        Extended = 0b0001'0000
    };

    namespace Detail
    {
        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        class RangeStateInterval;

        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        std::vector<RangeStateInterval<ValueType, IntervalKind>> cutIntervals(
            RangeStateInterval<ValueType, IntervalKind> const& k,
            RangeStateInterval<ValueType, IntervalKind> const& m);

        template <typename ValueType, typename IntervalKind>
        class RangeStateInterval
        {
          public:
            using value_type = ValueType;
            using interval_kind = IntervalKind;

          public:
            RangeStateInterval(value_type low, value_type high, RangeStateType type)
                : low_{low}
                , high_{high}
                , type_{type}
            {}
            void reset(value_type low, value_type high, RangeStateType type)
            {
                low_ = low;
                high_ = high;
                type_ = type;
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
            std::vector<RangeStateInterval> join(RangeStateInterval const& other) const
            {
                auto typeWithoutExtension = static_cast<RangeStateType>(other.type_ & ~RangeStateType::Extended);
                long extensionFix = other.type_ & RangeStateType::Extended ? 1 : 0;
                switch (typeWithoutExtension | type_)
                {
                    case (RangeStateType::Modify | RangeStateType::Modify):
                    {
                        return {
                            {std::min(low_, other.low_ + extensionFix),
                             std::max(high_, other.high_ - extensionFix),
                             RangeStateType::Modify}};
                    }
                    case (RangeStateType::Keep | RangeStateType::Modify):
                    {
                        // Modifications cut out of the keep part.
                        return cutIntervals(
                            *this, {other.low_ + extensionFix, other.high_ - extensionFix, typeWithoutExtension});
                    }
                    default:
                    {
                        std::cout << "Invalid insertion case: " << std::bitset<4>(typeWithoutExtension | type_) << "\n";
                        throw std::runtime_error("Invalid insertion case");
                    }
                }
            }
            RangeStateType type() const noexcept
            {
                return type_;
            }

          private:
            value_type low_;
            value_type high_;
            RangeStateType type_;
        };

        // stream iterator for intervals
        template <typename T, typename Kind>
        std::ostream& operator<<(std::ostream& os, RangeStateInterval<T, Kind> const& interval)
        {
            os << "[" << interval.low() << ", " << interval.high() << "]";
            switch (interval.type())
            {
                case RangeStateType::Keep:
                    os << "k";
                    break;
                case RangeStateType::Modify:
                    os << "m";
                    break;
                case RangeStateType::Insert:
                    os << "i";
                    break;
                default:
                    os << "?";
                    break;
            }
            return os;
        }

        template <typename ValueType, typename IntervalKind>
        std::vector<RangeStateInterval<ValueType, IntervalKind>> cutIntervals(
            RangeStateInterval<ValueType, IntervalKind> const& k,
            RangeStateInterval<ValueType, IntervalKind> const& m)
        {
            std::vector<RangeStateInterval<ValueType, IntervalKind>> result;
            if (k.low() <= m.low() - 1)
                result.emplace_back(k.low(), m.low() - 1, RangeStateType::Keep);
            result.emplace_back(m.low(), m.high(), RangeStateType::Modify);
            if (k.high() >= m.high() + 1)
                result.emplace_back(m.high() + 1, k.high(), RangeStateType::Keep);
            return result;
        }
    }

    class RangeEventContext
    {
      public:
        RangeEventContext(long dataSize)
            : modificationRanges_{}
            , fullRangeUpdate_{true}
        {
            reset(dataSize, true);
        }
        enum class InsertResult
        {
            Final, // Final, cannot accept further updates, must update immediately
            Accepted, // Accepted, update can be deferred.
            Retry // Must update immediately and reissue this.
        };
        InsertResult insertModificationRange(long elementCount, long low, long high, RangeStateType type)
        {
            if (type == RangeStateType::Erase)
            {
                // FIXME: optimize erase like insert.
                reset(elementCount, true);
                return InsertResult::Final;
            }
            else if (type == RangeStateType::Insert)
            {
                if (modificationRanges_.size() > 1)
                    return InsertResult::Retry;
                if (!insertInterval_)
                {
                    insertInterval_ = {low, high, type};
                    return InsertResult::Accepted;
                }
                else
                {
                    if (insertInterval_->overlapsOrIsAdjacent({low, high, type}))
                    {
                        auto lowmin = std::min(low, insertInterval_->low());
                        insertInterval_->reset(lowmin, lowmin + insertInterval_->size() + (high - low + 1), type);
                        return InsertResult::Accepted;
                    }
                    else
                    {
                        if (high < insertInterval_->low())
                        {
                            const auto size = high - low + 1;
                            insertInterval_->reset(
                                insertInterval_->low() + size, insertInterval_->high() + size, insertInterval_->type());
                        }
                        return InsertResult::Retry;
                    }
                }
            }
            if (insertInterval_)
                return InsertResult::Retry;

            auto iter = modificationRanges_.insert_overlap(
                {low - 1, high + 1, static_cast<RangeStateType>(type | RangeStateType::Extended)}, false, true);
            return InsertResult::Accepted;
        }
        InsertResult
        insertModificationRange(std::size_t elementCount, std::size_t low, std::size_t high, RangeStateType type)
        {
            return insertModificationRange(
                static_cast<long>(elementCount), static_cast<long>(low), static_cast<long>(high), type);
        }
        void reset(long dataSize, bool requireFullRangeUpdate)
        {
            modificationRanges_.clear();
            if (dataSize > 0)
                modificationRanges_.insert({0l, dataSize - 1, RangeStateType::Keep});
            insertInterval_ = std::nullopt;
            fullRangeUpdate_ = requireFullRangeUpdate;
        }
        bool isFullRangeUpdate() const noexcept
        {
            return fullRangeUpdate_;
        }
        std::optional<Detail::RangeStateInterval<long>> const& insertInterval() const
        {
            return insertInterval_;
        }
        auto begin() const
        {
            return modificationRanges_.begin();
        }
        auto end() const
        {
            return modificationRanges_.end();
        }

      private:
        lib_interval_tree::interval_tree<Detail::RangeStateInterval<long>> modificationRanges_;
        std::optional<Detail::RangeStateInterval<long>> insertInterval_;
        bool fullRangeUpdate_;
    };
}