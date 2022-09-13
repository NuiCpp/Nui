#pragma once

#include <interval-tree/interval_tree.hpp>

#include <cstddef>
#include <algorithm>
#include <vector>

namespace Nui
{
    namespace Detail
    {
        enum RangeStateType
        {
            Keep,
            Modify,
            Insert,
            Erase
        };

        template <typename ValueType, typename IntervalKind = lib_interval_tree::closed>
        class RangeStateInterval
        {
          public:
            using value_type = ValueType;
            using interval_kind = IntervalKind;

          public:
            RangeStateInterval(RangeStateType type, value_type low, value_type high)
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
                // TODO: special merge behavior with erase.
                return {{std::min(low_, other.low_), std::max(high_, other.high_), RangeStateType::Keep}};
            }

          private:
            value_type low_;
            value_type high_;
            RangeStateType type_;
        };
    }
    struct RangeEventContext
    {
        lib_interval_tree::interval_tree<RangeStateInterval<std::size_t>>;
    };
}