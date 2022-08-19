#include <nui/dom/dom.hpp>
#include <nui/utility/inferance_helper.hpp>
#include <nui/elements/div.hpp>

#include <emscripten/val.h>

namespace Nui::Dom
{
    //#####################################################################################################################
    Dom::Dom()
        : root_{Utility::InferanceHelper<div_>{}, emscripten::val::global("document")["body"]}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Element& Dom::root()
    {
        return root_;
    }
    //#####################################################################################################################
}