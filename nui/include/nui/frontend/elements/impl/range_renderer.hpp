#pragma once

#include <nui/frontend/event_system/range.hpp>
#include <nui/frontend/event_system/event_context.hpp>
#include <nui/frontend/dom/element_fwd.hpp>

#include <memory>

namespace Nui::Detail
{
    template <typename RangeType, typename GeneratorT, bool RandomAccess>
    class RangeRenderer;

    template <typename RangeLike, typename GeneratorT, typename... ObservedT>
    class UnoptimizedRangeRenderer;
}