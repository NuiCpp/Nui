#pragma once

#include <nui/frontend/val.hpp>

namespace Nui
{
    enum class ResizeableEdge
    {
        Right,
        Bottom
    };

    void makeResizeable(Nui::val const& element, ResizeableEdge edge = ResizeableEdge::Right);
}