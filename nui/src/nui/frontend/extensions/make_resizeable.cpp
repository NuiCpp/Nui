#include <nui/frontend/extensions/make_resizeable.hpp>

namespace Nui
{
    void makeResizeable(Nui::val const& element, ResizeableEdge edge)
    {
        const auto edgeString = [&edge]() -> std::string {
            if (edge == ResizeableEdge::Right)
                return "right";
            else if (edge == ResizeableEdge::Bottom)
                return "bottom";
            else if (edge == ResizeableEdge::Top)
                return "top";
            else
                return "right";
        }();

        Nui::val::global("nui_lib")["makeResizeable"](element, Nui::val{5}, Nui::val{edgeString});
    }
}