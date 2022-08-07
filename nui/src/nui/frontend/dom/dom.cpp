#include <nui/dom/dom.hpp>
#include <nui/elements/div.hpp>

#include <emscripten/val.h>

namespace Nui::Dom
{
    Dom::Dom()
        : root_{std::void_t<div_>{}, emscripten::val::global("document").call<emscripten::val>("createElement", "div")}
    {}
}