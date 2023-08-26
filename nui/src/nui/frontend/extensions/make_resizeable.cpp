#include <nui/frontend/extensions/make_resizeable.hpp>

namespace Nui
{
    void makeResizeable(Nui::val const& element, ResizeableEdge edge)
    {
        Nui::val::global("nui_lib")["makeResizeable"](
            element,
            Nui::val{5},
            Nui::val{edge == ResizeableEdge::Right ? std::string{"right"} : std::string{"bottom"}});
    }
}