#include <nui/dom/dom.hpp>
#include <nui/utility/inferance_helper.hpp>
#include <nui/elements/div.hpp>

#include <emscripten/val.h>

namespace Nui::Dom
{
    //#####################################################################################################################
    Dom::Dom()
        : root_{std::make_shared<Element>(emscripten::val::global("document")["body"])}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Element& Dom::root()
    {
        return *root_;
    }
    //#####################################################################################################################
}