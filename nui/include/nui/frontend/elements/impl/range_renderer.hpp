#pragma once

#include <nui/event_system/range.hpp>
#include <nui/event_system/event_context.hpp>
#include <nui/frontend/dom/element_fwd.hpp>
#include <nui/utility/scope_exit.hpp>
#include <nui/utility/overloaded.hpp>

#include <memory>
#include <utility>

namespace Nui::Detail
{
    template <typename RangeType, typename GeneratorT, bool RandomAccess>
    class RangeRenderer;

    template <typename RangeLike, typename GeneratorT, typename... ObservedT>
    class UnoptimizedRangeRenderer;
}