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
        Erase = 0b1000
    };

    namespace Detail
    {
        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        class RangeStateInterval;

        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        std::vector<RangeStateInterval<ValueType, IntervalKind>> cutIntervals(
            RangeStateInterval<ValueType, IntervalKind> const& inTree,
            RangeStateInterval<ValueType, IntervalKind> const& other,
            RangeStateType typeOfBeingCut,
            RangeStateType typeOfCutting);

        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        std::vector<RangeStateInterval<ValueType, IntervalKind>> joinExpandingInsertion(
            RangeStateInterval<ValueType, IntervalKind> const& inTree,
            RangeStateInterval<ValueType, IntervalKind> const& other,
            RangeStateType type);

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
                // in context of insert_overlap:
                // this = ival in the tree
                // other = interval that is inserted
                // merge set intervals are becoming other again.
                switch (other.type_ | type_)
                {
                    case (RangeStateType::Keep | RangeStateType::Keep):
                    {
                        return {{std::min(low_, other.low_), std::max(high_, other.high_), RangeStateType::Keep}};
                    }
                    case (RangeStateType::Modify | RangeStateType::Modify):
                    {
                        return {{std::min(low_, other.low_), std::max(high_, other.high_), RangeStateType::Modify}};
                    }
                    case (RangeStateType::Keep | RangeStateType::Modify):
                    {
                        // Modifications cut out of the keep part.
                        return cutIntervals(*this, other, RangeStateType::Keep, RangeStateType::Modify);
                    }
                    case (RangeStateType::Insert | RangeStateType::Insert):
                    {
                        // Inserts cannot lose size by merge, but must extend to the full size.
                        return {{std::min(low_, other.low_), size() + other.size(), RangeStateType::Insert}};
                    }
                    case (RangeStateType::Keep | RangeStateType::Insert):
                    {
                        // Inserts push off of keep intervals
                        return joinExpandingInsertion(*this, other, RangeStateType::Keep);
                    }
                    case (RangeStateType::Modify | RangeStateType::Insert):
                    {
                        if (other.type_ == RangeStateType::Modify && within(other))
                        {
                            // Modifications on insertions are ignored.
                            return {*this};
                        }
                        else
                        {
                            // Insertions push off of modifications.
                            return joinExpandingInsertion(*this, other, RangeStateType::Modify);
                        }
                    }
                        // case (RangeStateType::Erase | RangeStateType::Erase):
                        // {
                        //     // Erases cannot lose size by merge, but must extend to the full size.
                        //     // Because consecutive erases will erase their full range
                        //     return {{std::min(low_, other.low_), size() + other.size(), RangeStateType::Erase}};
                        // }
                        // case (RangeStateType::Erase | RangeStateType::Keep):
                        // {
                        //     // Erases cut out of the keep part.
                        //     return cutIntervals(*this, other, RangeStateType::Keep, RangeStateType::Erase);
                        // }
                        // case (RangeStateType::Erase | RangeStateType::Modify):
                        // {
                        //     // Modifications on erase sections should not happen
                        //     if (other.type_ == RangeStateType::Modify && within(other))
                        //     {
                        //         // Modifications on insertions are ignored.
                        //         // TODO: this will probably happen...
                        //         throw std::logic_error("Modification on erased section");
                        //     }
                        //     // Erases cut out of the modify part.
                        //     return cutIntervals(*this, other, RangeStateType::Modify, RangeStateType::Erase);
                        // }
                        // case (RangeStateType::Erase | RangeStateType::Insert):
                        // {
                        //     // Erases cut out of the insert part.
                        //     return cutIntervals(*this, other, RangeStateType::Insert, RangeStateType::Erase);
                        // }
                    default:
                    {
                        std::cout << "Invalid insertion case: " << std::bitset<4>(other.type_ | type_) << "\n";
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

        template <typename ValueType, typename IntervalKind>
        std::vector<RangeStateInterval<ValueType, IntervalKind>> cutIntervals(
            RangeStateInterval<ValueType, IntervalKind> const& inTree,
            RangeStateInterval<ValueType, IntervalKind> const& other,
            RangeStateType typeOfBeingCut,
            RangeStateType typeOfCutting)
        {
            auto const& k = other.type() == typeOfBeingCut ? other : inTree;
            auto const& m = other.type() == typeOfCutting ? other : inTree;
            std::vector<RangeStateInterval<ValueType, IntervalKind>> result;
            if (k.low() <= m.low() - 1)
                result.emplace_back(k.low(), m.low() - 1, typeOfBeingCut);
            result.emplace_back(m.low(), m.high(), typeOfCutting);
            if (k.high() >= m.high() + 1)
                result.emplace_back(m.high() + 1, k.high(), typeOfBeingCut);
            return result;
        }

        template <typename ValueType, typename IntervalKind>
        std::vector<RangeStateInterval<ValueType, IntervalKind>> joinExpandingInsertion(
            RangeStateInterval<ValueType, IntervalKind> const& inTree,
            RangeStateInterval<ValueType, IntervalKind> const& other,
            RangeStateType type)
        {
            auto extendedInterval = RangeStateInterval<ValueType, IntervalKind>{
                std::min(inTree.low(), other.low()), inTree.size() + other.size(), type};
            auto const& i = other.type() == RangeStateType::Insert ? other : inTree;
            return cutIntervals(extendedInterval, i, type, RangeStateType::Insert);
        }
    }

    class RangeEventContext
    {
      public:
        RangeEventContext(long dataSize)
            : modificationRanges_{}
            , eraseInterval_{}
            , fullRangeUpdate_{true}
        {
            if (dataSize > 0)
                modificationRanges_.insert({0l, dataSize, RangeStateType::Keep});
        }
        enum class InsertResult
        {
            Final, // Final, cannot accept further updates, must update immediately
            Accepted // Accepted, update can be deferred.
        };
        InsertResult insertModificationRange(long elementCount, long low, long high, RangeStateType type)
        {
            // TODO: improve: multiple mergeable erases are fine. So check with eraseInterval_.
            if (type == RangeStateType::Erase)
            {
                // eraseInterval_ = {low, high, type};
                // FIXME: optimize erase, uncomment above.
                reset(elementCount, true);
                return InsertResult::Final;
            }

            modificationRanges_.insert_overlap({low, high, type});
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
            modificationRanges_.insert({0l, dataSize, RangeStateType::Keep});
            eraseInterval_ = std::nullopt;
            fullRangeUpdate_ = requireFullRangeUpdate;
        }
        bool isFullRangeUpdate() const noexcept
        {
            return fullRangeUpdate_;
        }
        std::optional<Detail::RangeStateInterval<long>> const& eraseInterval() const
        {
            return eraseInterval_;
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
        // FIXME: insertions would be faster on vector<> while performing binary_search for insert_overlap.
        lib_interval_tree::interval_tree<Detail::RangeStateInterval<long>> modificationRanges_;
        std::optional<Detail::RangeStateInterval<long>> eraseInterval_;
        bool fullRangeUpdate_;
    };
}